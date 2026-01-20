use wasm_bindgen::prelude::*;

pub mod formatter;
pub mod highlighter;
pub mod types;
pub mod validator;

#[cfg(test)]
mod tests;

// Re-export public types for convenience (Rust API)
pub use formatter::{format_json, minify_json};
pub use highlighter::highlight_json;
pub use types::{FormatError, IndentStyle, JsonStats, ValidationResult};
pub use validator::validate_json;

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
