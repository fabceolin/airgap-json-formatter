use aes_gcm::aead::{Aead, KeyInit};
use aes_gcm::{Aes256Gcm, Key, Nonce};
use base64::{engine::general_purpose::URL_SAFE_NO_PAD, Engine as _};
use flate2::read::DeflateDecoder;
use flate2::write::DeflateEncoder;
use flate2::Compression;
use std::fmt;
use std::io::{Read, Write};

// ============================================================================
// Constants
// ============================================================================

pub const VERSION_RANDOM_KEY: u8 = 0x01;
pub const VERSION_PASSPHRASE: u8 = 0x02;
const PBKDF2_ITERATIONS: u32 = 100_000;
const SALT_LENGTH: usize = 16;
const NONCE_LENGTH: usize = 12;
const HEADER_LENGTH: usize = 9; // 1 version + 8 timestamp
const EXPIRATION_SECS: u64 = 300; // 5 minutes
const MAX_PAYLOAD_CHARS: usize = 6000;
const MAX_DECOMPRESSED_SIZE: usize = 10 * 1024 * 1024; // 10 MB

// ============================================================================
// Types
// ============================================================================

#[derive(Debug, Clone)]
pub struct SharePayload {
    pub data: String,
    pub key: Option<String>,
}

#[derive(Debug, Clone)]
pub struct DecodeResult {
    pub json: String,
    pub created_at: u64,
    pub mode: String,
}

#[derive(Debug, Clone, PartialEq)]
pub enum ShareError {
    CompressionFailed,
    EncryptionFailed,
    PayloadTooLarge,
    EmptyInput,
    KeyDerivationFailed,
    Expired,
    InvalidPayload,
    DecryptionFailed,
    InvalidBase64,
}

impl fmt::Display for ShareError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ShareError::CompressionFailed => write!(f, "Compression failed"),
            ShareError::EncryptionFailed => write!(f, "Encryption failed"),
            ShareError::PayloadTooLarge => write!(f, "Payload too large (max 6000 chars encoded)"),
            ShareError::EmptyInput => write!(f, "Input is empty"),
            ShareError::KeyDerivationFailed => write!(f, "Key derivation failed"),
            ShareError::Expired => {
                write!(
                    f,
                    "This shared link has expired (links are valid for 5 minutes)"
                )
            }
            ShareError::InvalidPayload => write!(f, "Invalid share link format"),
            ShareError::DecryptionFailed => {
                write!(f, "Unable to decrypt - the link may be corrupted")
            }
            ShareError::InvalidBase64 => write!(f, "Invalid share link encoding"),
        }
    }
}

impl ShareError {
    pub fn error_code(&self) -> &str {
        match self {
            ShareError::Expired => "expired",
            ShareError::InvalidPayload => "invalid_payload",
            ShareError::DecryptionFailed => "decryption_failed",
            ShareError::InvalidBase64 => "invalid_base64",
            ShareError::CompressionFailed => "invalid_payload",
            ShareError::EncryptionFailed => "decryption_failed",
            ShareError::PayloadTooLarge => "invalid_payload",
            ShareError::EmptyInput => "invalid_payload",
            ShareError::KeyDerivationFailed => "decryption_failed",
        }
    }
}

// ============================================================================
// Timestamp (injectable for testing)
// ============================================================================

#[cfg(not(test))]
fn get_unix_timestamp() -> u64 {
    (js_sys::Date::now() / 1000.0) as u64
}

#[cfg(test)]
fn get_unix_timestamp() -> u64 {
    MOCK_TIMESTAMP.with(|t| {
        let v = *t.borrow();
        if v == 0 {
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_secs()
        } else {
            v
        }
    })
}

#[cfg(test)]
thread_local! {
    static MOCK_TIMESTAMP: std::cell::RefCell<u64> = const { std::cell::RefCell::new(0) };
}

#[cfg(test)]
fn set_mock_timestamp(ts: u64) {
    MOCK_TIMESTAMP.with(|t| *t.borrow_mut() = ts);
}

// ============================================================================
// Encoding functions (Story 9.1 prerequisites)
// ============================================================================

pub fn encode_base64url(data: &[u8]) -> String {
    URL_SAFE_NO_PAD.encode(data)
}

fn compress_with_header(json: &str, version: u8) -> Result<Vec<u8>, ShareError> {
    let timestamp = get_unix_timestamp();
    let mut header = Vec::with_capacity(HEADER_LENGTH + json.len());
    header.push(version);
    header.extend_from_slice(&timestamp.to_be_bytes());
    header.extend_from_slice(json.as_bytes());

    let mut encoder = DeflateEncoder::new(Vec::new(), Compression::default());
    encoder
        .write_all(&header)
        .map_err(|_| ShareError::CompressionFailed)?;
    encoder.finish().map_err(|_| ShareError::CompressionFailed)
}

pub fn derive_key_from_passphrase(
    passphrase: &str,
    salt: &[u8],
) -> Result<[u8; 32], ShareError> {
    let mut key = [0u8; 32];
    pbkdf2::pbkdf2_hmac::<sha2::Sha256>(passphrase.as_bytes(), salt, PBKDF2_ITERATIONS, &mut key);
    Ok(key)
}

fn encrypt_payload(data: &[u8], key_bytes: &[u8; 32]) -> Result<Vec<u8>, ShareError> {
    let key = Key::<Aes256Gcm>::from_slice(key_bytes);
    let cipher = Aes256Gcm::new(key);

    let mut nonce_bytes = [0u8; NONCE_LENGTH];
    getrandom::getrandom(&mut nonce_bytes).map_err(|_| ShareError::EncryptionFailed)?;
    let nonce = Nonce::from_slice(&nonce_bytes);

    let ciphertext = cipher
        .encrypt(nonce, data)
        .map_err(|_| ShareError::EncryptionFailed)?;

    let mut result = nonce_bytes.to_vec();
    result.extend(ciphertext);
    Ok(result)
}

pub fn generate_random_key() -> Result<[u8; 32], ShareError> {
    let mut key = [0u8; 32];
    getrandom::getrandom(&mut key).map_err(|_| ShareError::EncryptionFailed)?;
    Ok(key)
}

pub fn create_share_payload(
    json: &str,
    passphrase: Option<&str>,
) -> Result<SharePayload, ShareError> {
    if json.is_empty() {
        return Err(ShareError::EmptyInput);
    }

    let is_passphrase = passphrase.is_some_and(|p| !p.is_empty());
    let version = if is_passphrase {
        VERSION_PASSPHRASE
    } else {
        VERSION_RANDOM_KEY
    };

    let compressed = compress_with_header(json, version)?;

    if is_passphrase {
        let passphrase = passphrase.unwrap();
        let mut salt = [0u8; SALT_LENGTH];
        getrandom::getrandom(&mut salt).map_err(|_| ShareError::EncryptionFailed)?;
        let key = derive_key_from_passphrase(passphrase, &salt)?;
        let encrypted = encrypt_payload(&compressed, &key)?;

        let mut payload = salt.to_vec();
        payload.extend(encrypted);

        let data = encode_base64url(&payload);
        if data.len() > MAX_PAYLOAD_CHARS {
            return Err(ShareError::PayloadTooLarge);
        }
        Ok(SharePayload { data, key: None })
    } else {
        let key_bytes = generate_random_key()?;
        let encrypted = encrypt_payload(&compressed, &key_bytes)?;
        let data = encode_base64url(&encrypted);
        if data.len() > MAX_PAYLOAD_CHARS {
            return Err(ShareError::PayloadTooLarge);
        }
        Ok(SharePayload {
            data,
            key: Some(encode_base64url(&key_bytes)),
        })
    }
}

// ============================================================================
// Decoding functions (Story 9.2)
// ============================================================================

pub fn decode_base64url(input: &str) -> Result<Vec<u8>, ShareError> {
    URL_SAFE_NO_PAD
        .decode(input)
        .map_err(|_| ShareError::InvalidBase64)
}

pub fn decrypt_payload(ciphertext: &[u8], key_bytes: &[u8]) -> Result<Vec<u8>, ShareError> {
    if key_bytes.len() != 32 {
        return Err(ShareError::InvalidPayload);
    }
    if ciphertext.len() < NONCE_LENGTH + 1 {
        return Err(ShareError::InvalidPayload);
    }

    let (nonce_bytes, encrypted) = ciphertext.split_at(NONCE_LENGTH);
    let nonce = Nonce::from_slice(nonce_bytes);
    let key = Key::<Aes256Gcm>::from_slice(key_bytes);
    let cipher = Aes256Gcm::new(key);

    cipher
        .decrypt(nonce, encrypted)
        .map_err(|_| ShareError::DecryptionFailed)
}

pub fn extract_header(data: &[u8]) -> Result<(u8, u64, &[u8]), ShareError> {
    if data.len() < HEADER_LENGTH {
        return Err(ShareError::InvalidPayload);
    }
    let version = data[0];
    let timestamp = u64::from_be_bytes(
        data[1..9]
            .try_into()
            .map_err(|_| ShareError::InvalidPayload)?,
    );
    Ok((version, timestamp, &data[HEADER_LENGTH..]))
}

fn validate_timestamp(created_at: u64) -> Result<(), ShareError> {
    let now = get_unix_timestamp();
    if now.saturating_sub(created_at) > EXPIRATION_SECS {
        return Err(ShareError::Expired);
    }
    Ok(())
}

/// Decompress DEFLATE data to raw bytes (for wire format with binary header)
pub fn decompress_raw(compressed: &[u8]) -> Result<Vec<u8>, ShareError> {
    let mut decoder = DeflateDecoder::new(compressed);
    let mut decompressed = Vec::new();

    // Read in chunks to enforce size limit
    let mut buf = [0u8; 8192];
    loop {
        let n = decoder
            .read(&mut buf)
            .map_err(|_| ShareError::CompressionFailed)?;
        if n == 0 {
            break;
        }
        decompressed.extend_from_slice(&buf[..n]);
        if decompressed.len() > MAX_DECOMPRESSED_SIZE {
            return Err(ShareError::InvalidPayload);
        }
    }

    Ok(decompressed)
}

/// Decompress DEFLATE data and convert to UTF-8 string (for pure JSON payloads)
pub fn decompress_json(compressed: &[u8]) -> Result<String, ShareError> {
    let decompressed = decompress_raw(compressed)?;
    String::from_utf8(decompressed).map_err(|_| ShareError::InvalidPayload)
}

pub fn decode_share_payload(
    data: &str,
    key_or_passphrase: &str,
    is_passphrase: bool,
) -> Result<DecodeResult, ShareError> {
    let raw = decode_base64url(data)?;

    let (decrypted, expected_version) = if is_passphrase {
        // Passphrase mode: [salt:16][nonce:12][ciphertext...]
        if raw.len() < SALT_LENGTH + NONCE_LENGTH + 1 {
            return Err(ShareError::InvalidPayload);
        }
        let (salt, ciphertext) = raw.split_at(SALT_LENGTH);
        let key = derive_key_from_passphrase(key_or_passphrase, salt)?;
        let decrypted = decrypt_payload(ciphertext, &key)?;
        (decrypted, VERSION_PASSPHRASE)
    } else {
        // Random key mode: [nonce:12][ciphertext...]
        let key_bytes = decode_base64url(key_or_passphrase)?;
        if key_bytes.len() != 32 {
            return Err(ShareError::InvalidPayload);
        }
        let decrypted = decrypt_payload(&raw, &key_bytes)?;
        (decrypted, VERSION_RANDOM_KEY)
    };

    // Decompress to raw bytes: [version:1][timestamp:8][json_bytes...]
    let decompressed = decompress_raw(&decrypted)?;

    // Extract binary header from raw bytes
    let (version, timestamp, json_bytes) = extract_header(&decompressed)?;

    if version != expected_version {
        return Err(ShareError::InvalidPayload);
    }

    validate_timestamp(timestamp)?;

    // Convert JSON portion (after header) to UTF-8 string
    let json = String::from_utf8(json_bytes.to_vec()).map_err(|_| ShareError::InvalidPayload)?;

    let mode = if version == VERSION_RANDOM_KEY {
        "quick"
    } else {
        "protected"
    };

    Ok(DecodeResult {
        json,
        created_at: timestamp,
        mode: mode.to_string(),
    })
}

// ============================================================================
// Tests
// ============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    fn reset_mock() {
        set_mock_timestamp(0);
    }

    // --- Task 1: DecodeResult and ShareError ---

    #[test]
    fn test_decode_result_fields() {
        let result = DecodeResult {
            json: r#"{"a":1}"#.to_string(),
            created_at: 1706367600,
            mode: "quick".to_string(),
        };
        assert_eq!(result.json, r#"{"a":1}"#);
        assert_eq!(result.created_at, 1706367600);
        assert_eq!(result.mode, "quick");
    }

    #[test]
    fn test_share_error_display() {
        assert_eq!(
            ShareError::Expired.to_string(),
            "This shared link has expired (links are valid for 5 minutes)"
        );
        assert_eq!(
            ShareError::InvalidPayload.to_string(),
            "Invalid share link format"
        );
        assert_eq!(
            ShareError::DecryptionFailed.to_string(),
            "Unable to decrypt - the link may be corrupted"
        );
        assert_eq!(
            ShareError::InvalidBase64.to_string(),
            "Invalid share link encoding"
        );
    }

    #[test]
    fn test_error_codes() {
        assert_eq!(ShareError::Expired.error_code(), "expired");
        assert_eq!(ShareError::InvalidPayload.error_code(), "invalid_payload");
        assert_eq!(ShareError::DecryptionFailed.error_code(), "decryption_failed");
        assert_eq!(ShareError::InvalidBase64.error_code(), "invalid_base64");
    }

    // --- Task 2: Base64URL decoding ---

    #[test]
    fn test_base64url_roundtrip() {
        let data = b"hello world";
        let encoded = encode_base64url(data);
        let decoded = decode_base64url(&encoded).unwrap();
        assert_eq!(decoded, data);
    }

    #[test]
    fn test_base64url_no_padding() {
        let encoded = encode_base64url(b"ab");
        assert!(!encoded.contains('='));
    }

    #[test]
    fn test_base64url_url_safe_chars() {
        // Encode bytes that would produce + and / in standard base64
        let data: Vec<u8> = (0..=255).collect();
        let encoded = encode_base64url(&data);
        assert!(!encoded.contains('+'));
        assert!(!encoded.contains('/'));
    }

    #[test]
    fn test_base64url_invalid_input() {
        let result = decode_base64url("not valid base64!!!");
        assert_eq!(result.unwrap_err(), ShareError::InvalidBase64);
    }

    // --- Task 3: PBKDF2 key derivation ---

    #[test]
    fn test_pbkdf2_deterministic() {
        let salt = [0u8; 16];
        let key1 = derive_key_from_passphrase("test", &salt).unwrap();
        let key2 = derive_key_from_passphrase("test", &salt).unwrap();
        assert_eq!(key1, key2);
    }

    #[test]
    fn test_pbkdf2_different_passphrase() {
        let salt = [0u8; 16];
        let key1 = derive_key_from_passphrase("test1", &salt).unwrap();
        let key2 = derive_key_from_passphrase("test2", &salt).unwrap();
        assert_ne!(key1, key2);
    }

    #[test]
    fn test_pbkdf2_different_salt() {
        let salt1 = [0u8; 16];
        let salt2 = [1u8; 16];
        let key1 = derive_key_from_passphrase("test", &salt1).unwrap();
        let key2 = derive_key_from_passphrase("test", &salt2).unwrap();
        assert_ne!(key1, key2);
    }

    // --- Task 4: AES-256-GCM decryption ---

    #[test]
    fn test_encrypt_decrypt_roundtrip() {
        let mut key = [0u8; 32];
        getrandom::getrandom(&mut key).unwrap();
        let plaintext = b"hello world";
        let encrypted = encrypt_payload(plaintext, &key).unwrap();
        let decrypted = decrypt_payload(&encrypted, &key).unwrap();
        assert_eq!(decrypted, plaintext);
    }

    #[test]
    fn test_decrypt_wrong_key() {
        let mut key1 = [0u8; 32];
        let mut key2 = [1u8; 32];
        getrandom::getrandom(&mut key1).unwrap();
        getrandom::getrandom(&mut key2).unwrap();
        let encrypted = encrypt_payload(b"data", &key1).unwrap();
        let result = decrypt_payload(&encrypted, &key2);
        assert_eq!(result.unwrap_err(), ShareError::DecryptionFailed);
    }

    #[test]
    fn test_decrypt_too_short() {
        let result = decrypt_payload(&[0u8; 5], &[0u8; 32]);
        assert_eq!(result.unwrap_err(), ShareError::InvalidPayload);
    }

    #[test]
    fn test_decrypt_invalid_key_length() {
        let result = decrypt_payload(&[0u8; 20], &[0u8; 16]);
        assert_eq!(result.unwrap_err(), ShareError::InvalidPayload);
    }

    // --- Task 5: Version byte and timestamp ---

    #[test]
    fn test_extract_header_valid() {
        let mut data = vec![0x01];
        data.extend_from_slice(&1706367600u64.to_be_bytes());
        data.extend_from_slice(b"remaining");
        let (version, ts, rest) = extract_header(&data).unwrap();
        assert_eq!(version, 0x01);
        assert_eq!(ts, 1706367600);
        assert_eq!(rest, b"remaining");
    }

    #[test]
    fn test_extract_header_too_short() {
        let result = extract_header(&[0u8; 5]);
        assert_eq!(result.unwrap_err(), ShareError::InvalidPayload);
    }

    #[test]
    fn test_expiration_boundary_not_expired() {
        reset_mock();
        let now = get_unix_timestamp();
        set_mock_timestamp(now);

        // 299 seconds ago: should succeed
        let ts_299 = now - 299;
        let result = validate_timestamp(ts_299);
        assert!(result.is_ok(), "299s should not be expired");
    }

    #[test]
    fn test_expiration_boundary_exactly_300() {
        reset_mock();
        let now = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap()
            .as_secs();
        set_mock_timestamp(now);

        // 300 seconds ago: should succeed (> 300 to expire)
        let ts_300 = now - 300;
        let result = validate_timestamp(ts_300);
        assert!(result.is_ok(), "300s should not be expired (boundary)");
    }

    #[test]
    fn test_expiration_boundary_expired() {
        reset_mock();
        let now = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap()
            .as_secs();
        set_mock_timestamp(now);

        // 301 seconds ago: should expire
        let ts_301 = now - 301;
        let result = validate_timestamp(ts_301);
        assert_eq!(result.unwrap_err(), ShareError::Expired);
    }

    // --- Task 6: DEFLATE decompression ---

    #[test]
    fn test_compress_decompress_roundtrip() {
        reset_mock();
        let json = r#"{"key": "value", "nested": {"a": 1}}"#;
        let version = VERSION_RANDOM_KEY;
        let compressed = compress_with_header(json, version).unwrap();
        // Use decompress_raw since wire format has binary header (not valid UTF-8)
        let decompressed = decompress_raw(&compressed).unwrap();

        // decompressed = [version:1][timestamp:8][json...]
        assert_eq!(decompressed[0], version);
        let json_part = &decompressed[HEADER_LENGTH..];
        assert_eq!(std::str::from_utf8(json_part).unwrap(), json);
    }

    #[test]
    fn test_decompress_invalid_data() {
        let result = decompress_json(&[0xFF, 0xFE, 0xFD]);
        assert!(result.is_err());
    }

    // --- Task 7 & 9: Full round-trip integration tests ---

    #[test]
    fn test_roundtrip_random_key_mode() {
        reset_mock();
        let json = r#"{"test": "data", "count": 42}"#;
        let payload = create_share_payload(json, None).unwrap();
        assert!(payload.key.is_some());

        let result =
            decode_share_payload(&payload.data, payload.key.as_ref().unwrap(), false).unwrap();
        assert_eq!(result.json, json);
        assert_eq!(result.mode, "quick");
    }

    #[test]
    fn test_roundtrip_passphrase_mode() {
        reset_mock();
        let json = r#"{"secret": "value"}"#;
        let passphrase = "my-secret-pass";
        let payload = create_share_payload(json, Some(passphrase)).unwrap();
        assert!(payload.key.is_none());

        let result = decode_share_payload(&payload.data, passphrase, true).unwrap();
        assert_eq!(result.json, json);
        assert_eq!(result.mode, "protected");
    }

    #[test]
    fn test_expired_link() {
        reset_mock();
        // Create payload at time T
        let now = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap()
            .as_secs();
        set_mock_timestamp(now);
        let json = r#"{"test": true}"#;
        let payload = create_share_payload(json, None).unwrap();

        // Decode at time T + 301
        set_mock_timestamp(now + 301);
        let result =
            decode_share_payload(&payload.data, payload.key.as_ref().unwrap(), false);
        assert_eq!(result.unwrap_err(), ShareError::Expired);
    }

    #[test]
    fn test_tampered_data() {
        reset_mock();
        let json = r#"{"test": "data"}"#;
        let payload = create_share_payload(json, None).unwrap();
        let mut raw = decode_base64url(&payload.data).unwrap();
        // Flip a bit in the ciphertext (after nonce)
        if raw.len() > 20 {
            raw[20] ^= 0xFF;
        }
        let tampered_data = encode_base64url(&raw);
        let result =
            decode_share_payload(&tampered_data, payload.key.as_ref().unwrap(), false);
        assert_eq!(result.unwrap_err(), ShareError::DecryptionFailed);
    }

    #[test]
    fn test_wrong_key() {
        reset_mock();
        let json = r#"{"test": "data"}"#;
        let payload = create_share_payload(json, None).unwrap();
        // Generate a different random key
        let mut wrong_key = [0u8; 32];
        getrandom::getrandom(&mut wrong_key).unwrap();
        let wrong_key_b64 = encode_base64url(&wrong_key);
        let result = decode_share_payload(&payload.data, &wrong_key_b64, false);
        assert_eq!(result.unwrap_err(), ShareError::DecryptionFailed);
    }

    #[test]
    fn test_wrong_passphrase() {
        reset_mock();
        let json = r#"{"test": "data"}"#;
        let payload = create_share_payload(json, Some("correct-pass")).unwrap();
        let result = decode_share_payload(&payload.data, "wrong-pass", true);
        assert_eq!(result.unwrap_err(), ShareError::DecryptionFailed);
    }

    #[test]
    fn test_mode_mismatch_key_as_passphrase() {
        reset_mock();
        let json = r#"{"test": "data"}"#;
        let payload = create_share_payload(json, None).unwrap();
        // Try to decode random-key payload as passphrase mode
        let result = decode_share_payload(&payload.data, "some-pass", true);
        assert!(result.is_err());
    }

    #[test]
    fn test_truncated_data() {
        reset_mock();
        // Only a few bytes - too short to be a valid payload
        let short = encode_base64url(&[0u8; 5]);
        let mut key = [0u8; 32];
        getrandom::getrandom(&mut key).unwrap();
        let key_b64 = encode_base64url(&key);
        let result = decode_share_payload(&short, &key_b64, false);
        assert!(result.is_err());
    }

    #[test]
    fn test_invalid_base64_input() {
        let result = decode_share_payload("!!!invalid!!!", "key", false);
        assert_eq!(result.unwrap_err(), ShareError::InvalidBase64);
    }

    #[test]
    fn test_empty_input_encoding() {
        let result = create_share_payload("", None);
        assert_eq!(result.unwrap_err(), ShareError::EmptyInput);
    }

    #[test]
    fn test_large_json_roundtrip() {
        reset_mock();
        let json = format!(r#"{{"data": "{}"}}"#, "x".repeat(1000));
        let payload = create_share_payload(&json, None).unwrap();
        let result =
            decode_share_payload(&payload.data, payload.key.as_ref().unwrap(), false).unwrap();
        assert_eq!(result.json, json);
    }

    // --- Task 6: Random key generation ---

    #[test]
    fn test_generate_random_key_returns_32_bytes() {
        let key = generate_random_key().unwrap();
        assert_eq!(key.len(), 32);
    }

    #[test]
    fn test_generate_random_key_unique() {
        let key1 = generate_random_key().unwrap();
        let key2 = generate_random_key().unwrap();
        assert_ne!(key1, key2, "Random keys should be unique");
    }

    // --- GAP-1 (AC10): PayloadTooLarge test ---

    #[test]
    fn test_payload_too_large() {
        reset_mock();
        // Generate JSON large enough to exceed 6000 chars when encoded
        // Base64 expands by ~4/3, so 6000 chars = ~4500 bytes
        // After header (9 bytes), nonce (12 bytes), tag (16 bytes) overhead
        // Use truly random data as hex string to prevent compression
        let mut random_bytes = [0u8; 5000];
        getrandom::getrandom(&mut random_bytes).unwrap();
        let hex_data: String = random_bytes.iter().map(|b| format!("{:02x}", b)).collect();
        let large_json = format!(r#"{{"data":"{}"}}"#, hex_data);
        let result = create_share_payload(&large_json, None);
        assert_eq!(result.unwrap_err(), ShareError::PayloadTooLarge);
    }

    #[test]
    fn test_payload_too_large_passphrase_mode() {
        reset_mock();
        // Passphrase mode adds 16-byte salt, so threshold is similar
        let mut random_bytes = [0u8; 5000];
        getrandom::getrandom(&mut random_bytes).unwrap();
        let hex_data: String = random_bytes.iter().map(|b| format!("{:02x}", b)).collect();
        let large_json = format!(r#"{{"data":"{}"}}"#, hex_data);
        let result = create_share_payload(&large_json, Some("passphrase"));
        assert_eq!(result.unwrap_err(), ShareError::PayloadTooLarge);
    }

    // --- GAP-4: Nonce uniqueness test ---

    #[test]
    fn test_nonce_uniqueness_different_ciphertexts() {
        reset_mock();
        let json = r#"{"same": "data"}"#;
        let payload1 = create_share_payload(json, None).unwrap();
        let payload2 = create_share_payload(json, None).unwrap();
        // Data should be different due to different nonces
        assert_ne!(payload1.data, payload2.data, "Same input should produce different ciphertext (random nonce)");
    }

    // --- GAP-5: Unicode round-trip test ---

    #[test]
    fn test_unicode_emoji_roundtrip() {
        reset_mock();
        let json = r#"{"emoji":"🎉","cjk":"日本語","arabic":"مرحبا"}"#;
        let payload = create_share_payload(json, None).unwrap();
        let result =
            decode_share_payload(&payload.data, payload.key.as_ref().unwrap(), false).unwrap();
        assert_eq!(result.json, json, "Unicode should round-trip losslessly");
    }

    #[test]
    fn test_unicode_emoji_roundtrip_passphrase() {
        reset_mock();
        let json = r#"{"emoji":"🎉🚀","text":"你好世界"}"#;
        let passphrase = "unicode-pass-🔐";
        let payload = create_share_payload(json, Some(passphrase)).unwrap();
        let result = decode_share_payload(&payload.data, passphrase, true).unwrap();
        assert_eq!(result.json, json, "Unicode should round-trip with passphrase");
    }
}
