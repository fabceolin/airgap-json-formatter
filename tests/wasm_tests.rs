//! WASM integration tests using wasm-bindgen-test
//!
//! Run with: wasm-pack test --headless --chrome
//! or: wasm-pack test --headless --firefox

#![cfg(target_arch = "wasm32")]

use wasm_bindgen_test::*;

wasm_bindgen_test_configure!(run_in_browser);

use airgap_json_formatter::{js_format_json, js_highlight_json, js_minify_json, js_validate_json, greet};

#[wasm_bindgen_test]
fn test_greet() {
    let result = greet();
    assert_eq!(result, "Airgap JSON Formatter loaded successfully!");
}

#[wasm_bindgen_test]
fn test_format_json_spaces_2() {
    let input = r#"{"name":"John","age":30}"#;
    let result = js_format_json(input, "spaces:2").unwrap();
    assert!(result.contains("  \""));
    assert!(result.contains("\"name\""));
}

#[wasm_bindgen_test]
fn test_format_json_spaces_4() {
    let input = r#"{"key":"value"}"#;
    let result = js_format_json(input, "spaces:4").unwrap();
    assert!(result.contains("    \"key\""));
}

#[wasm_bindgen_test]
fn test_format_json_tabs() {
    let input = r#"{"key":"value"}"#;
    let result = js_format_json(input, "tabs").unwrap();
    assert!(result.contains("\t\"key\""));
}

#[wasm_bindgen_test]
fn test_format_json_invalid_input() {
    let input = "{invalid}";
    let result = js_format_json(input, "spaces:2");
    assert!(result.is_err());
}

#[wasm_bindgen_test]
fn test_format_json_invalid_indent() {
    let input = r#"{"key":"value"}"#;
    let result = js_format_json(input, "invalid");
    assert!(result.is_err());
}

#[wasm_bindgen_test]
fn test_minify_json() {
    let input = r#"{
        "name": "John",
        "age": 30
    }"#;
    let result = js_minify_json(input).unwrap();
    assert!(!result.contains('\n'));
    assert!(!result.contains("  "));
}

#[wasm_bindgen_test]
fn test_minify_json_invalid() {
    let input = "{invalid}";
    let result = js_minify_json(input);
    assert!(result.is_err());
}

#[wasm_bindgen_test]
fn test_validate_json_valid() {
    let input = r#"{"name":"test"}"#;
    let result = js_validate_json(input);
    assert!(result.contains("\"isValid\":true"));
    assert!(result.contains("\"error\":null"));
}

#[wasm_bindgen_test]
fn test_validate_json_invalid() {
    let input = "{invalid}";
    let result = js_validate_json(input);
    assert!(result.contains("\"isValid\":false"));
    assert!(result.contains("\"error\":{"));
    assert!(result.contains("\"line\":"));
    assert!(result.contains("\"column\":"));
}

#[wasm_bindgen_test]
fn test_validate_json_stats() {
    let input = r#"{"a":1,"b":[1,2],"c":true}"#;
    let result = js_validate_json(input);
    assert!(result.contains("\"objectCount\":1"));
    assert!(result.contains("\"arrayCount\":1"));
    assert!(result.contains("\"numberCount\":3")); // 1, 1, 2
    assert!(result.contains("\"booleanCount\":1"));
}

#[wasm_bindgen_test]
fn test_round_trip() {
    // Format then minify should produce consistent output
    let original = r#"{"name":"test","value":42}"#;
    let formatted = js_format_json(original, "spaces:2").unwrap();
    let minified = js_minify_json(&formatted).unwrap();

    // Minified should be valid
    let validation = js_validate_json(&minified);
    assert!(validation.contains("\"isValid\":true"));
}

#[wasm_bindgen_test]
fn test_highlight_json_basic() {
    let input = r#"{"key": "value", "num": 42}"#;
    let result = js_highlight_json(input);
    assert!(result.contains("<span")); // Has HTML spans
    assert!(result.contains("key"));
    assert!(result.contains("value"));
}

#[wasm_bindgen_test]
fn test_highlight_empty_input() {
    let result = js_highlight_json("");
    assert!(result.is_empty());
}

#[wasm_bindgen_test]
fn test_highlight_all_json_types() {
    let input = r#"{"str": "hello", "num": 123, "bool": true, "nil": null}"#;
    let result = js_highlight_json(input);
    assert!(result.contains("<span"));
    assert!(result.contains("hello"));
    assert!(result.contains("123"));
    assert!(result.contains("true"));
    assert!(result.contains("null"));
}
