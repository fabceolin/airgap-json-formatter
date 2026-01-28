//! XML Syntax Highlighter
//!
//! Production-grade syntax highlighting for XML documents using a state machine parser.
//! Generates HTML with inline styles for rendering in the browser.
//!
//! # Features
//!
//! - **Token-accurate highlighting**: Tags, attributes, values, comments, CDATA, declarations,
//!   entities, and text content each get distinct colors
//! - **VS Code dark theme colors**: Consistent with the widely-used editor theme
//! - **XSS protection**: All 5 HTML special characters (`<`, `>`, `&`, `"`, `'`) are escaped
//! - **Graceful degradation**: Malformed XML (unclosed tags, comments, CDATA, attributes)
//!   produces valid HTML with partial highlighting in contextually correct colors
//! - **Performance**: O(n) single-pass state machine, handles 100KB+ in <100ms
//! - **Input limits**: Rejects inputs >5MB to prevent OOM in WASM
//!
//! # Color Palette (VS Code Dark Theme)
//!
//! | Token Type | Color | Hex Code |
//! |------------|-------|----------|
//! | Tag names | Blue | `#569cd6` |
//! | Attribute names | Light blue | `#9cdcfe` |
//! | Attribute values | Orange | `#ce9178` |
//! | Text content | Gray | `#d4d4d4` |
//! | Comments | Green | `#6a9955` |
//! | CDATA sections | Yellow | `#dcdcaa` |
//! | Declarations (<?xml ?>, <!DOCTYPE>) | Purple | `#c586c0` |
//! | Brackets (`<`, `>`, `/>`) | Gray | `#808080` |
//! | Entity references (`&amp;`, etc.) | Gold | `#d7ba7d` |
//!
//! # Graceful Degradation
//!
//! When processing malformed XML, the highlighter ensures:
//! - **Unclosed tags**: Tag name highlighted in TAG color, buffer flushed at EOF
//! - **Unclosed comments**: Remainder highlighted in COMMENT color
//! - **Unclosed CDATA**: Remainder highlighted in CDATA color
//! - **Unclosed attributes**: Value highlighted in ATTR_VALUE color
//! - **Invalid chars after `<`**: Skipped with `<` rendered as bracket
//!
//! The highlighter never crashes or hangs on malformed input.
//!
//! # Input Size Limit
//!
//! Inputs exceeding 5MB are rejected with an error message to prevent out-of-memory
//! conditions in the WASM environment. The limit is checked before any allocation.

/// Color palette (VS Code dark theme)
mod colors {
    /// Blue - tag names (`<root>`, `</root>`)
    pub const TAG: &str = "#569cd6";
    /// Light blue - attribute names (`id`, `class`)
    pub const ATTR_NAME: &str = "#9cdcfe";
    /// Orange - attribute values (`"value"`)
    pub const ATTR_VALUE: &str = "#ce9178";
    /// Gray - text content between tags
    pub const TEXT: &str = "#d4d4d4";
    /// Green - XML comments (`<!-- ... -->`)
    pub const COMMENT: &str = "#6a9955";
    /// Yellow - CDATA sections (`<![CDATA[ ... ]]>`)
    pub const CDATA: &str = "#dcdcaa";
    /// Purple - XML declarations and doctypes (`<?xml ?>`, `<!DOCTYPE>`)
    pub const DECLARATION: &str = "#c586c0";
    /// Gray - angle brackets (`<`, `>`, `/>`)
    pub const BRACKET: &str = "#808080";
    /// Gold - entity references (`&amp;`, `&lt;`, etc.)
    pub const ENTITY: &str = "#d7ba7d";
}

/// Parser state
#[derive(Debug, Clone, Copy, PartialEq)]
enum State {
    Text,
    TagOpen,        // Just saw <
    TagName,        // Reading tag name
    TagClose,       // Reading </tagname
    InTag,          // Inside tag, between attributes
    AttrName,       // Reading attribute name
    AttrEquals,     // After = before value
    AttrValue,      // Reading attribute value
    Comment,        // Inside <!-- -->
    Cdata,          // Inside <![CDATA[ ]]>
    Declaration,    // Inside <?xml ?>
    Doctype,        // Inside <!DOCTYPE >
}

/// Maximum input size (5MB) to prevent OOM in WASM
const MAX_INPUT_SIZE: usize = 5 * 1024 * 1024;

/// Highlights XML string and returns HTML with inline styles.
pub fn highlight_xml(input: &str) -> String {
    if input.is_empty() {
        return String::new();
    }

    // Size guard before allocation to prevent OOM on large inputs
    if input.len() > MAX_INPUT_SIZE {
        return "<pre style=\"color:#f44336\">Error: Input exceeds 5MB limit</pre>".to_string();
    }

    let mut output = String::with_capacity(input.len() * 3);
    output.push_str("<pre style=\"margin:0;font-family:inherit;\">");

    let chars: Vec<char> = input.chars().collect();
    let len = chars.len();
    let mut i = 0;
    let mut state = State::Text;
    let mut buffer = String::new();
    let mut quote_char: Option<char> = None;

    while i < len {
        let c = chars[i];

        match state {
            State::Text => {
                if c == '<' {
                    // Flush text buffer
                    if !buffer.is_empty() {
                        push_colored_escaped(&mut output, &buffer, colors::TEXT);
                        buffer.clear();
                    }
                    state = State::TagOpen;
                    i += 1;
                } else if c == '&' {
                    // Entity reference
                    if !buffer.is_empty() {
                        push_colored_escaped(&mut output, &buffer, colors::TEXT);
                        buffer.clear();
                    }
                    let (entity, end) = parse_entity(&chars, i);
                    push_colored_escaped(&mut output, &entity, colors::ENTITY);
                    i = end;
                } else {
                    buffer.push(c);
                    i += 1;
                }
            }

            State::TagOpen => {
                if c == '!' {
                    // Could be comment, CDATA, or DOCTYPE
                    if matches_str(&chars, i, "!--") {
                        push_colored(&mut output, "&lt;!--", colors::COMMENT);
                        state = State::Comment;
                        i += 3;
                    } else if matches_str(&chars, i, "![CDATA[") {
                        push_colored(&mut output, "&lt;![CDATA[", colors::CDATA);
                        state = State::Cdata;
                        i += 8;
                    } else if matches_str(&chars, i, "!DOCTYPE") {
                        push_colored(&mut output, "&lt;!DOCTYPE", colors::DECLARATION);
                        state = State::Doctype;
                        i += 8;
                    } else {
                        push_colored(&mut output, "&lt;!", colors::BRACKET);
                        state = State::Text;
                        i += 1;
                    }
                } else if c == '?' {
                    push_colored(&mut output, "&lt;?", colors::DECLARATION);
                    state = State::Declaration;
                    i += 1;
                } else if c == '/' {
                    push_colored(&mut output, "&lt;/", colors::BRACKET);
                    state = State::TagClose;
                    i += 1;
                } else if c.is_alphabetic() || c == '_' || c == ':' {
                    push_colored(&mut output, "&lt;", colors::BRACKET);
                    buffer.push(c);
                    state = State::TagName;
                    i += 1;
                } else {
                    push_colored(&mut output, "&lt;", colors::BRACKET);
                    state = State::Text;
                    i += 1; // Explicit: skip unrecognized char after < (control chars are invalid XML)
                }
            }

            State::TagName => {
                if c.is_alphanumeric() || c == '_' || c == '-' || c == '.' || c == ':' {
                    buffer.push(c);
                    i += 1;
                } else {
                    // Flush tag name
                    push_colored_escaped(&mut output, &buffer, colors::TAG);
                    buffer.clear();
                    if c == '>' {
                        push_colored(&mut output, "&gt;", colors::BRACKET);
                        state = State::Text;
                        i += 1;
                    } else if c == '/' {
                        if i + 1 < len && chars[i + 1] == '>' {
                            push_colored(&mut output, "/&gt;", colors::BRACKET);
                            state = State::Text;
                            i += 2;
                        } else {
                            output.push('/');
                            i += 1;
                        }
                    } else {
                        state = State::InTag;
                    }
                }
            }

            State::TagClose => {
                if c.is_alphanumeric() || c == '_' || c == '-' || c == '.' || c == ':' {
                    buffer.push(c);
                    i += 1;
                } else if c == '>' {
                    push_colored_escaped(&mut output, &buffer, colors::TAG);
                    buffer.clear();
                    push_colored(&mut output, "&gt;", colors::BRACKET);
                    state = State::Text;
                    i += 1;
                } else {
                    push_colored_escaped(&mut output, &buffer, colors::TAG);
                    buffer.clear();
                    state = State::InTag;
                }
            }

            State::InTag => {
                if c.is_whitespace() {
                    output.push(c);
                    i += 1;
                } else if c == '>' {
                    push_colored(&mut output, "&gt;", colors::BRACKET);
                    state = State::Text;
                    i += 1;
                } else if c == '/' {
                    if i + 1 < len && chars[i + 1] == '>' {
                        push_colored(&mut output, "/&gt;", colors::BRACKET);
                        state = State::Text;
                        i += 2;
                    } else {
                        output.push('/');
                        i += 1;
                    }
                } else if c.is_alphabetic() || c == '_' || c == ':' {
                    buffer.push(c);
                    state = State::AttrName;
                    i += 1;
                } else {
                    output.push(c);
                    i += 1;
                }
            }

            State::AttrName => {
                if c.is_alphanumeric() || c == '_' || c == '-' || c == '.' || c == ':' {
                    buffer.push(c);
                    i += 1;
                } else {
                    push_colored_escaped(&mut output, &buffer, colors::ATTR_NAME);
                    buffer.clear();
                    if c == '=' {
                        output.push('=');
                        state = State::AttrEquals;
                        i += 1;
                    } else {
                        state = State::InTag;
                    }
                }
            }

            State::AttrEquals => {
                if c == '"' || c == '\'' {
                    quote_char = Some(c);
                    buffer.push(c);
                    state = State::AttrValue;
                    i += 1;
                } else if c.is_whitespace() {
                    output.push(c);
                    i += 1;
                } else {
                    state = State::InTag;
                }
            }

            State::AttrValue => {
                if Some(c) == quote_char {
                    buffer.push(c);
                    push_colored_escaped(&mut output, &buffer, colors::ATTR_VALUE);
                    buffer.clear();
                    quote_char = None;
                    state = State::InTag;
                    i += 1;
                } else {
                    buffer.push(c);
                    i += 1;
                }
            }

            State::Comment => {
                if matches_str(&chars, i, "-->") {
                    if !buffer.is_empty() {
                        push_colored_escaped(&mut output, &buffer, colors::COMMENT);
                        buffer.clear();
                    }
                    push_colored(&mut output, "--&gt;", colors::COMMENT);
                    state = State::Text;
                    i += 3;
                } else {
                    buffer.push(c);
                    i += 1;
                }
            }

            State::Cdata => {
                if matches_str(&chars, i, "]]>") {
                    if !buffer.is_empty() {
                        push_colored_escaped(&mut output, &buffer, colors::CDATA);
                        buffer.clear();
                    }
                    push_colored(&mut output, "]]&gt;", colors::CDATA);
                    state = State::Text;
                    i += 3;
                } else {
                    buffer.push(c);
                    i += 1;
                }
            }

            State::Declaration => {
                if matches_str(&chars, i, "?>") {
                    if !buffer.is_empty() {
                        push_colored_escaped(&mut output, &buffer, colors::DECLARATION);
                        buffer.clear();
                    }
                    push_colored(&mut output, "?&gt;", colors::DECLARATION);
                    state = State::Text;
                    i += 2;
                } else {
                    buffer.push(c);
                    i += 1;
                }
            }

            State::Doctype => {
                if c == '>' {
                    if !buffer.is_empty() {
                        push_colored_escaped(&mut output, &buffer, colors::DECLARATION);
                        buffer.clear();
                    }
                    push_colored(&mut output, "&gt;", colors::BRACKET);
                    state = State::Text;
                    i += 1;
                } else {
                    buffer.push(c);
                    i += 1;
                }
            }
        }
    }

    // Flush remaining buffer with contextually correct color per state
    if !buffer.is_empty() {
        let color = match state {
            State::Text => colors::TEXT,
            State::Comment => colors::COMMENT,
            State::Cdata => colors::CDATA,
            State::Declaration | State::Doctype => colors::DECLARATION,
            State::TagName | State::TagClose => colors::TAG,
            State::AttrName => colors::ATTR_NAME,
            State::AttrValue => colors::ATTR_VALUE,
            State::AttrEquals | State::InTag | State::TagOpen => colors::BRACKET,
        };
        push_colored_escaped(&mut output, &buffer, color);
    }

    output.push_str("</pre>");
    output
}

/// Check if a substring matches at position i
fn matches_str(chars: &[char], start: usize, pattern: &str) -> bool {
    let pattern_chars: Vec<char> = pattern.chars().collect();
    if start + pattern_chars.len() > chars.len() {
        return false;
    }
    for (j, pc) in pattern_chars.iter().enumerate() {
        if chars[start + j] != *pc {
            return false;
        }
    }
    true
}

/// Parse entity reference starting with &
fn parse_entity(chars: &[char], start: usize) -> (String, usize) {
    let mut result = String::new();
    result.push('&');
    let mut i = start + 1;
    let len = chars.len();

    while i < len {
        let c = chars[i];
        result.push(c);
        if c == ';' {
            return (result, i + 1);
        }
        if !c.is_alphanumeric() && c != '#' {
            break;
        }
        i += 1;
    }

    (result, i)
}

/// Push colored HTML span with HTML escaping
fn push_colored_escaped(output: &mut String, text: &str, color: &str) {
    output.push_str("<span style=\"color:");
    output.push_str(color);
    output.push_str("\">");
    for c in text.chars() {
        match c {
            '<' => output.push_str("&lt;"),
            '>' => output.push_str("&gt;"),
            '&' => output.push_str("&amp;"),
            '"' => output.push_str("&quot;"),
            '\'' => output.push_str("&#39;"),
            _ => output.push(c),
        }
    }
    output.push_str("</span>");
}

/// Push colored HTML span (text already escaped).
///
/// SAFETY INVARIANT: This function MUST only be called with static strings or
/// pre-escaped content (e.g., known delimiters like "&lt;", "&gt;", "/&gt;").
/// NEVER call this function with user-derived content - use `push_colored_escaped()` instead.
fn push_colored(output: &mut String, text: &str, color: &str) {
    output.push_str("<span style=\"color:");
    output.push_str(color);
    output.push_str("\">");
    output.push_str(text);
    output.push_str("</span>");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_highlight_empty() {
        assert!(highlight_xml("").is_empty());
    }

    #[test]
    fn test_highlight_simple_element() {
        let result = highlight_xml("<root>text</root>");
        assert!(result.contains("root"));
        assert!(result.contains("text"));
        assert!(result.contains("<span"));
    }

    #[test]
    fn test_highlight_with_attributes() {
        let result = highlight_xml(r#"<elem attr="value"/>"#);
        assert!(result.contains("elem"));
        assert!(result.contains("attr"));
        assert!(result.contains("value"));
    }

    #[test]
    fn test_highlight_comment() {
        let result = highlight_xml("<!-- comment -->");
        assert!(result.contains("comment"));
        assert!(result.contains(colors::COMMENT));
    }

    #[test]
    fn test_highlight_cdata() {
        let result = highlight_xml("<![CDATA[raw data]]>");
        assert!(result.contains("raw data"));
        assert!(result.contains(colors::CDATA));
    }

    #[test]
    fn test_highlight_declaration() {
        let result = highlight_xml(r#"<?xml version="1.0"?>"#);
        assert!(result.contains("xml"));
        assert!(result.contains(colors::DECLARATION));
    }

    #[test]
    fn test_highlight_namespace() {
        let result = highlight_xml(r#"<ns:root xmlns:ns="http://example.com"/>"#);
        assert!(result.contains("ns:root"));
        assert!(result.contains("xmlns:ns"));
    }

    #[test]
    fn test_escapes_html() {
        let result = highlight_xml("<root><![CDATA[<script>]]></root>");
        assert!(!result.contains("<script>"));
        assert!(result.contains("&lt;script&gt;"));
    }

    // ========== Task 3: Malformed XML and Edge Case Tests ==========

    // P0: Test infinite loop regression - control char after < must terminate
    #[test]
    fn test_control_char_after_tag_open_terminates() {
        let result = highlight_xml("<\x01");
        // Must terminate (not hang) and return valid HTML
        assert!(result.contains("<pre"));
        assert!(result.contains("</pre>"));
        // The < should be escaped as &lt;
        assert!(result.contains("&lt;"));
    }

    // P0: Test null byte after < must terminate
    #[test]
    fn test_null_byte_after_tag_open_terminates() {
        let result = highlight_xml("<\x00");
        // Must terminate (not hang) and return valid HTML
        assert!(result.contains("<pre"));
        assert!(result.contains("</pre>"));
        // The < should be escaped as &lt;
        assert!(result.contains("&lt;"));
    }

    // P0: Unclosed tag flushes with TAG color
    #[test]
    fn test_unclosed_tag_flushes_with_tag_color() {
        let result = highlight_xml("<root");
        assert!(result.contains("<pre"));
        assert!(result.contains("</pre>"));
        // "root" should be in TAG color (#569cd6)
        assert!(result.contains(colors::TAG));
        assert!(result.contains("root"));
    }

    // P0: Unclosed comment flushes with COMMENT color
    #[test]
    fn test_unclosed_comment_flushes_with_comment_color() {
        let result = highlight_xml("<!-- comment");
        assert!(result.contains("<pre"));
        assert!(result.contains("</pre>"));
        // Remainder should be in COMMENT color (#6a9955)
        assert!(result.contains(colors::COMMENT));
        assert!(result.contains("comment"));
    }

    // P0: Unclosed CDATA flushes with CDATA color
    #[test]
    fn test_unclosed_cdata_flushes_with_cdata_color() {
        let result = highlight_xml("<![CDATA[data");
        assert!(result.contains("<pre"));
        assert!(result.contains("</pre>"));
        // Remainder should be in CDATA color (#dcdcaa)
        assert!(result.contains(colors::CDATA));
        assert!(result.contains("data"));
    }

    // P0: Unclosed attribute value flushes with ATTR_VALUE color
    #[test]
    fn test_unclosed_attr_value_flushes_with_attr_value_color() {
        let result = highlight_xml("<a b=\"value");
        assert!(result.contains("<pre"));
        assert!(result.contains("</pre>"));
        // "value should be in ATTR_VALUE color (#ce9178)
        assert!(result.contains(colors::ATTR_VALUE));
        assert!(result.contains("value"));
    }

    // P1: Incomplete entity (no semicolon) - valid HTML, no crash
    #[test]
    fn test_incomplete_entity_no_crash() {
        let result = highlight_xml("&amp");
        assert!(result.contains("<pre"));
        assert!(result.contains("</pre>"));
        // Entity should be in output (escaped)
        assert!(result.contains("&amp;amp")); // & becomes &amp;, then "amp" follows
    }

    // P1: Input exceeding 5MB limit returns error message
    #[test]
    fn test_input_exceeds_5mb_limit() {
        // Generate input slightly over 5MB
        let large_input: String = "x".repeat(5 * 1024 * 1024 + 1);
        let result = highlight_xml(&large_input);
        assert!(result.contains("Error: Input exceeds 5MB limit"));
        assert!(result.contains("#f44336")); // Error color
    }

    // ========== Task 4: XSS Protection Tests ==========

    // P0: Single-quoted attribute produces &#39; in output
    #[test]
    fn test_single_quote_escaped_in_attribute() {
        let result = highlight_xml("<a b='val'>");
        // Single quotes should be escaped as &#39;
        assert!(result.contains("&#39;"));
        // Should NOT contain unescaped single quote in span content
        // The raw ' character should not appear between > and <
        assert!(!result.contains(">val'<") && !result.contains(">'val"));
    }

    // P0: <script>alert(1)</script> fully escaped
    #[test]
    fn test_script_tag_xss_escaped() {
        let result = highlight_xml("<script>alert(1)</script>");
        // The <script> tag should be rendered as highlighted XML, not as executable HTML
        // Tag name "script" should be in output
        assert!(result.contains("script"));
        // All < and > should be escaped
        assert!(result.contains("&lt;"));
        assert!(result.contains("&gt;"));
        // No raw <script> tag should exist in output
        assert!(!result.contains("<script>"));
    }

    // P1: Attribute-context XSS (onclick handler) escaped
    #[test]
    fn test_onclick_attribute_xss_escaped() {
        let result = highlight_xml(r#"<a onclick="alert(1)">"#);
        // "onclick" should appear (as attribute name)
        assert!(result.contains("onclick"));
        // The quotes in value should be escaped
        assert!(result.contains("&quot;") || result.contains("&#34;"));
        // No raw double quote in attribute value context that could break out
        assert!(result.contains("alert(1)"));
    }

    // P1: All 5 HTML special chars individually verified in output
    #[test]
    fn test_all_five_special_chars_escaped() {
        // Test input with all 5 special chars in text content
        let result = highlight_xml("<root>Test: < > & \" '</root>");

        // Each special char should be escaped
        assert!(result.contains("&lt;")); // <
        assert!(result.contains("&gt;")); // >
        assert!(result.contains("&amp;")); // &
        assert!(result.contains("&quot;")); // "
        assert!(result.contains("&#39;")); // '
    }

    // ========== Task 5: Performance Tests ==========

    // Generate 100KB of valid XML for benchmarking
    fn generate_100kb_xml() -> String {
        let mut xml = String::with_capacity(110_000);
        xml.push_str("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        xml.push_str("<root>\n");

        // Each item is ~50 bytes, need ~2000 items for 100KB
        for i in 0..2000 {
            xml.push_str(&format!(
                "  <item id=\"{}\" attr=\"value{}\">Content text {}</item>\n",
                i, i, i
            ));
        }

        xml.push_str("</root>");
        xml
    }

    // P2: 100KB XML document highlights in < 100ms
    #[test]
    fn test_100kb_xml_performance() {
        use std::time::Instant;

        let xml = generate_100kb_xml();
        let input_size = xml.len();
        assert!(input_size >= 100_000, "Generated XML should be at least 100KB, got {} bytes", input_size);

        let start = Instant::now();
        let result = highlight_xml(&xml);
        let duration = start.elapsed();

        // Verify result is valid
        assert!(result.contains("<pre"));
        assert!(result.contains("</pre>"));
        assert!(result.contains(colors::TAG));

        // Performance assertion: must complete in < 100ms
        assert!(
            duration.as_millis() < 100,
            "100KB XML highlighting took {}ms, expected < 100ms",
            duration.as_millis()
        );

        // Log actual performance (visible with --nocapture)
        println!("Performance: {}KB input highlighted in {:?}", input_size / 1024, duration);
    }

    // P2: Memory usage verification (log allocation ratios)
    #[test]
    fn test_memory_usage_logging() {
        let xml = generate_100kb_xml();
        let input_size = xml.len();

        let result = highlight_xml(&xml);
        let output_size = result.len();

        // Due to HTML span tags wrapping each token, output will be significantly larger.
        // Each token gets ~40 chars of span overhead (<span style="color:#xxxxxx">...</span>)
        // A realistic ratio for heavily-tagged XML is 8-12x.
        let ratio = output_size as f64 / input_size as f64;

        // Log allocation sizes (visible with --nocapture)
        println!("Memory: input={}KB, output={}KB, ratio={:.2}x",
                 input_size / 1024, output_size / 1024, ratio);

        // Verify output is reasonable (not exponentially larger due to a bug)
        // Allow up to 15x for heavily tagged content with full highlighting
        assert!(
            ratio < 15.0,
            "Output/input ratio {:.2}x exceeds 15x limit. Input: {}KB, Output: {}KB. This may indicate a bug.",
            ratio, input_size / 1024, output_size / 1024
        );

        // Verify the output is valid HTML
        assert!(result.starts_with("<pre"));
        assert!(result.ends_with("</pre>"));
    }
}
