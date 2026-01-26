//! XML syntax highlighter - State machine implementation
//!
//! Provides syntax highlighting for XML using a simple state machine parser.
//! Mirrors the pattern from highlighter.rs for JSON.

/// Color palette (VS Code dark theme inspired)
mod colors {
    pub const TAG: &str = "#569cd6";           // Blue for tags
    pub const ATTR_NAME: &str = "#9cdcfe";     // Light blue for attribute names
    pub const ATTR_VALUE: &str = "#ce9178";    // Orange for attribute values
    pub const TEXT: &str = "#d4d4d4";          // Gray for text content
    pub const COMMENT: &str = "#6a9955";       // Green for comments
    pub const CDATA: &str = "#dcdcaa";         // Yellow for CDATA
    pub const DECLARATION: &str = "#c586c0";   // Purple for XML declarations
    pub const BRACKET: &str = "#808080";       // Gray for < > brackets
    pub const ENTITY: &str = "#d7ba7d";        // Gold for entities like &amp;
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

/// Highlights XML string and returns HTML with inline styles.
pub fn highlight_xml(input: &str) -> String {
    if input.is_empty() {
        return String::new();
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

    // Flush remaining buffer
    if !buffer.is_empty() {
        let color = match state {
            State::Text => colors::TEXT,
            State::Comment => colors::COMMENT,
            State::Cdata => colors::CDATA,
            State::Declaration | State::Doctype => colors::DECLARATION,
            _ => colors::TEXT,
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
            _ => output.push(c),
        }
    }
    output.push_str("</span>");
}

/// Push colored HTML span (text already escaped)
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
}
