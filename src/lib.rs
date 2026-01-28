use wasm_bindgen::prelude::*;

pub mod formatter;
pub mod highlighter;
pub mod markdown_highlighter;
pub mod markdown_renderer;
pub mod share;
pub mod types;
pub mod validator;
pub mod xml_formatter;
pub mod xml_highlighter;

#[cfg(test)]
mod tests;

// Re-export public types for convenience (Rust API)
pub use formatter::{format_json, minify_json};
pub use highlighter::highlight_json;
pub use types::{FormatError, IndentStyle, JsonStats, ValidationResult};
pub use validator::validate_json;
pub use xml_formatter::{format_xml, minify_xml};
pub use xml_highlighter::highlight_xml;
pub use markdown_highlighter::highlight_markdown;
pub use markdown_renderer::{render_markdown, RenderError};

// ============================================================================
// WASM/JavaScript API
// ============================================================================

/// Placeholder function to verify WASM binding works.
/// Returns a greeting message to confirm the module is loaded.
#[wasm_bindgen]
pub fn greet() -> String {
    "Airgap JSON Formatter loaded successfully!".to_string()
}

/// Parse indent style string into IndentStyle enum.
/// Accepts: "spaces:2", "spaces:4", "tabs"
fn parse_indent_style(indent: &str) -> Result<IndentStyle, JsValue> {
    match indent {
        "tabs" => Ok(IndentStyle::Tabs),
        s if s.starts_with("spaces:") => {
            let num = s.strip_prefix("spaces:")
                .and_then(|n| n.parse::<u8>().ok())
                .ok_or_else(|| JsValue::from_str("Invalid indent format. Use 'spaces:N' or 'tabs'"))?;
            Ok(IndentStyle::Spaces(num))
        }
        _ => Err(JsValue::from_str("Invalid indent format. Use 'spaces:2', 'spaces:4', or 'tabs'")),
    }
}

/// Format JSON with specified indentation.
///
/// # Arguments
/// * `input` - The JSON string to format
/// * `indent` - Indent style: "spaces:2", "spaces:4", or "tabs"
///
/// # Returns
/// * Formatted JSON string on success
/// * Throws error string on failure
#[wasm_bindgen(js_name = "formatJson")]
pub fn js_format_json(input: &str, indent: &str) -> Result<String, JsValue> {
    let style = parse_indent_style(indent)?;
    formatter::format_json(input, style)
        .map_err(|e| JsValue::from_str(&e.to_string()))
}

/// Minify JSON by removing all unnecessary whitespace.
///
/// # Arguments
/// * `input` - The JSON string to minify
///
/// # Returns
/// * Minified JSON string on success
/// * Throws error string on failure
#[wasm_bindgen(js_name = "minifyJson")]
pub fn js_minify_json(input: &str) -> Result<String, JsValue> {
    formatter::minify_json(input)
        .map_err(|e| JsValue::from_str(&e.to_string()))
}

/// Validate JSON and return statistics as JSON string.
///
/// # Arguments
/// * `input` - The JSON string to validate
///
/// # Returns
/// * JSON string containing validation result:
///   ```json
///   {
///     "isValid": boolean,
///     "error": { "message": string, "line": number, "column": number } | null,
///     "stats": {
///       "objectCount": number,
///       "arrayCount": number,
///       "stringCount": number,
///       "numberCount": number,
///       "booleanCount": number,
///       "nullCount": number,
///       "maxDepth": number,
///       "totalKeys": number
///     }
///   }
///   ```
#[wasm_bindgen(js_name = "validateJson")]
pub fn js_validate_json(input: &str) -> String {
    let result = validator::validate_json(input);

    // Serialize to JavaScript-friendly JSON
    let error_json = match &result.error {
        Some(e) => format!(
            r#"{{"message":"{}","line":{},"column":{}}}"#,
            e.message.replace('\\', "\\\\").replace('"', "\\\""),
            e.line,
            e.column
        ),
        None => "null".to_string(),
    };

    format!(
        r#"{{"isValid":{},"error":{},"stats":{{"objectCount":{},"arrayCount":{},"stringCount":{},"numberCount":{},"booleanCount":{},"nullCount":{},"maxDepth":{},"totalKeys":{}}}}}"#,
        result.is_valid,
        error_json,
        result.stats.object_count,
        result.stats.array_count,
        result.stats.string_count,
        result.stats.number_count,
        result.stats.boolean_count,
        result.stats.null_count,
        result.stats.max_depth,
        result.stats.total_keys
    )
}

/// Highlight JSON with syntax colors, returning HTML with inline styles.
///
/// # Arguments
/// * `input` - The JSON string to highlight
///
/// # Returns
/// * HTML string with inline styles for syntax highlighting
/// * Empty string if input is empty
/// * Escaped plain text if highlighting fails
#[wasm_bindgen(js_name = "highlightJson")]
pub fn js_highlight_json(input: &str) -> String {
    highlighter::highlight_json(input)
}

// ============================================================================
// XML WASM Exports (Spike - Q1 Investigation)
// ============================================================================

/// Format XML with specified indentation.
///
/// # Arguments
/// * `input` - The XML string to format
/// * `indent` - Indent style: "spaces:2", "spaces:4", or "tabs"
///
/// # Returns
/// * Formatted XML string on success
/// * Throws error string on failure
#[wasm_bindgen(js_name = "formatXml")]
pub fn js_format_xml(input: &str, indent: &str) -> Result<String, JsValue> {
    let style = parse_indent_style(indent)?;
    xml_formatter::format_xml(input, style)
        .map_err(|e| JsValue::from_str(&e.to_string()))
}

/// Minify XML by removing all unnecessary whitespace.
///
/// # Arguments
/// * `input` - The XML string to minify
///
/// # Returns
/// * Minified XML string on success
/// * Throws error string on failure
#[wasm_bindgen(js_name = "minifyXml")]
pub fn js_minify_xml(input: &str) -> Result<String, JsValue> {
    xml_formatter::minify_xml(input)
        .map_err(|e| JsValue::from_str(&e.to_string()))
}

/// Highlight XML with syntax colors, returning HTML with inline styles.
///
/// # Arguments
/// * `input` - The XML string to highlight
///
/// # Returns
/// * HTML string with inline styles for syntax highlighting
#[wasm_bindgen(js_name = "highlightXml")]
pub fn js_highlight_xml(input: &str) -> String {
    xml_highlighter::highlight_xml(input)
}

// ============================================================================
// Share WASM Exports
// ============================================================================

/// Decode a shared payload, returning JSON with result or error.
#[wasm_bindgen(js_name = "decodeSharePayload")]
pub fn js_decode_share_payload(data: &str, key_or_passphrase: &str, is_passphrase: bool) -> String {
    match share::decode_share_payload(data, key_or_passphrase, is_passphrase) {
        Ok(result) => {
            format!(
                r#"{{"success":true,"json":{},"createdAt":{},"mode":"{}"}}"#,
                serde_json::to_string(&result.json).unwrap_or_else(|_| format!("\"{}\"", result.json)),
                result.created_at,
                result.mode
            )
        }
        Err(e) => {
            let error_code = match &e {
                share::ShareError::DecryptionFailed if is_passphrase => "wrong_passphrase",
                other => other.error_code(),
            };
            format!(
                r#"{{"success":false,"error":"{}","errorCode":"{}"}}"#,
                e, error_code
            )
        }
    }
}

/// Create a share payload (encoding), returning JSON with result or error.
#[wasm_bindgen(js_name = "createSharePayload")]
pub fn js_create_share_payload(json: &str, passphrase: &str) -> String {
    let pass = if passphrase.is_empty() {
        None
    } else {
        Some(passphrase)
    };
    match share::create_share_payload(json, pass) {
        Ok(payload) => {
            match payload.key {
                Some(key) => format!(
                    r#"{{"success":true,"data":"{}","key":"{}","mode":"quick"}}"#,
                    payload.data, key
                ),
                None => format!(
                    r#"{{"success":true,"data":"{}","mode":"protected"}}"#,
                    payload.data
                ),
            }
        }
        Err(e) => {
            format!(r#"{{"success":false,"error":"{}"}}"#, e)
        }
    }
}

// ============================================================================
// Markdown WASM Exports
// ============================================================================

/// Highlight Markdown with syntax colors, returning HTML with inline styles.
///
/// # Arguments
/// * `input` - The Markdown string to highlight
///
/// # Returns
/// * HTML string with inline styles for syntax highlighting
#[wasm_bindgen(js_name = "highlightMarkdown")]
pub fn js_highlight_markdown(input: &str) -> String {
    markdown_highlighter::highlight_markdown(input)
}

/// Render Markdown to HTML with GFM extensions.
///
/// # Arguments
/// * `input` - The Markdown string to render
///
/// # Returns
/// * HTML string on success
/// * Error HTML div with escaped message on failure (AC11)
///
/// # Security
/// - Input size limited to 2MB (AC13)
/// - Dangerous URI schemes sanitized (AC12)
/// - Error messages HTML-escaped to prevent XSS
#[wasm_bindgen(js_name = "renderMarkdown")]
pub fn js_render_markdown(input: &str) -> String {
    match markdown_renderer::render_markdown(input) {
        Ok(html) => html,
        Err(e) => {
            // AC11: HTML-escape error message to prevent XSS via error path
            let escaped = e.message
                .replace('&', "&amp;")
                .replace('<', "&lt;")
                .replace('>', "&gt;")
                .replace('"', "&quot;");
            format!("<div class=\"error\">{}</div>", escaped)
        }
    }
}
