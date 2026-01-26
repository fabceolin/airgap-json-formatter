//! XML Formatting Module - Spike Investigation
//!
//! This module evaluates quick-xml for WASM compatibility and basic formatting capabilities.

use quick_xml::events::{BytesEnd, BytesStart, BytesText, Event};
use quick_xml::{Reader, Writer};
use std::io::Cursor;

use crate::types::{FormatError, IndentStyle};

/// Format XML with specified indentation.
///
/// # Arguments
/// * `input` - The XML string to format
/// * `indent` - Indentation style (spaces or tabs)
///
/// # Returns
/// * Formatted XML string on success
/// * FormatError on failure
pub fn format_xml(input: &str, indent: IndentStyle) -> Result<String, FormatError> {
    if input.trim().is_empty() {
        return Err(FormatError::new("Empty input", 0, 0));
    }

    let indent_char = match indent {
        IndentStyle::Spaces(_) => b' ',
        IndentStyle::Tabs => b'\t',
    };
    let indent_size = match indent {
        IndentStyle::Spaces(n) => n as usize,
        IndentStyle::Tabs => 1,
    };

    let mut reader = Reader::from_str(input);
    reader.config_mut().trim_text_start = true;
    reader.config_mut().trim_text_end = true;

    let mut writer = Writer::new_with_indent(Cursor::new(Vec::new()), indent_char, indent_size);
    let mut buf = Vec::new();

    loop {
        match reader.read_event_into(&mut buf) {
            Ok(Event::Start(e)) => {
                let name = String::from_utf8(e.name().as_ref().to_vec())
                    .map_err(|_| FormatError::new("Invalid UTF-8 in tag name", 0, 0))?;
                let mut new_elem = BytesStart::new(name);
                for attr in e.attributes() {
                    let attr = attr.map_err(|_| FormatError::new("Invalid attribute", 0, 0))?;
                    new_elem.push_attribute(attr);
                }
                writer
                    .write_event(Event::Start(new_elem))
                    .map_err(|e| FormatError::new(&format!("Write error: {}", e), 0, 0))?;
            }
            Ok(Event::End(e)) => {
                let name = String::from_utf8(e.name().as_ref().to_vec())
                    .map_err(|_| FormatError::new("Invalid UTF-8 in tag name", 0, 0))?;
                let end = BytesEnd::new(name);
                writer
                    .write_event(Event::End(end))
                    .map_err(|e| FormatError::new(&format!("Write error: {}", e), 0, 0))?;
            }
            Ok(Event::Empty(e)) => {
                let name = String::from_utf8(e.name().as_ref().to_vec())
                    .map_err(|_| FormatError::new("Invalid UTF-8 in tag name", 0, 0))?;
                let mut new_elem = BytesStart::new(name);
                for attr in e.attributes() {
                    let attr = attr.map_err(|_| FormatError::new("Invalid attribute", 0, 0))?;
                    new_elem.push_attribute(attr);
                }
                writer
                    .write_event(Event::Empty(new_elem))
                    .map_err(|e| FormatError::new(&format!("Write error: {}", e), 0, 0))?;
            }
            Ok(Event::Text(e)) => {
                let text = e
                    .unescape()
                    .map_err(|_| FormatError::new("Invalid text content", 0, 0))?;
                if !text.trim().is_empty() {
                    writer
                        .write_event(Event::Text(BytesText::new(&text)))
                        .map_err(|e| FormatError::new(&format!("Write error: {}", e), 0, 0))?;
                }
            }
            Ok(Event::CData(e)) => {
                writer
                    .write_event(Event::CData(e))
                    .map_err(|e| FormatError::new(&format!("Write error: {}", e), 0, 0))?;
            }
            Ok(Event::Comment(e)) => {
                writer
                    .write_event(Event::Comment(e))
                    .map_err(|e| FormatError::new(&format!("Write error: {}", e), 0, 0))?;
            }
            Ok(Event::Decl(e)) => {
                writer
                    .write_event(Event::Decl(e))
                    .map_err(|e| FormatError::new(&format!("Write error: {}", e), 0, 0))?;
            }
            Ok(Event::PI(e)) => {
                writer
                    .write_event(Event::PI(e))
                    .map_err(|e| FormatError::new(&format!("Write error: {}", e), 0, 0))?;
            }
            Ok(Event::DocType(e)) => {
                writer
                    .write_event(Event::DocType(e))
                    .map_err(|e| FormatError::new(&format!("Write error: {}", e), 0, 0))?;
            }
            Ok(Event::Eof) => break,
            Err(e) => {
                return Err(FormatError::new(&format!("XML parse error: {}", e), 0, 0));
            }
        }
        buf.clear();
    }

    let result = writer.into_inner().into_inner();
    String::from_utf8(result).map_err(|_| FormatError::new("Invalid UTF-8 in output", 0, 0))
}

/// Minify XML by removing unnecessary whitespace.
pub fn minify_xml(input: &str) -> Result<String, FormatError> {
    if input.trim().is_empty() {
        return Err(FormatError::new("Empty input", 0, 0));
    }

    let mut reader = Reader::from_str(input);
    reader.config_mut().trim_text_start = true;
    reader.config_mut().trim_text_end = true;

    let mut writer = Writer::new(Cursor::new(Vec::new()));
    let mut buf = Vec::new();

    loop {
        match reader.read_event_into(&mut buf) {
            Ok(Event::Start(e)) => {
                let name = String::from_utf8(e.name().as_ref().to_vec())
                    .map_err(|_| FormatError::new("Invalid UTF-8", 0, 0))?;
                let mut new_elem = BytesStart::new(name);
                for attr in e.attributes() {
                    let attr = attr.map_err(|_| FormatError::new("Invalid attribute", 0, 0))?;
                    new_elem.push_attribute(attr);
                }
                writer
                    .write_event(Event::Start(new_elem))
                    .map_err(|e| FormatError::new(&format!("Write error: {}", e), 0, 0))?;
            }
            Ok(Event::End(e)) => {
                let name = String::from_utf8(e.name().as_ref().to_vec())
                    .map_err(|_| FormatError::new("Invalid UTF-8", 0, 0))?;
                let end = BytesEnd::new(name);
                writer
                    .write_event(Event::End(end))
                    .map_err(|e| FormatError::new(&format!("Write error: {}", e), 0, 0))?;
            }
            Ok(Event::Empty(e)) => {
                let name = String::from_utf8(e.name().as_ref().to_vec())
                    .map_err(|_| FormatError::new("Invalid UTF-8", 0, 0))?;
                let mut new_elem = BytesStart::new(name);
                for attr in e.attributes() {
                    let attr = attr.map_err(|_| FormatError::new("Invalid attribute", 0, 0))?;
                    new_elem.push_attribute(attr);
                }
                writer
                    .write_event(Event::Empty(new_elem))
                    .map_err(|e| FormatError::new(&format!("Write error: {}", e), 0, 0))?;
            }
            Ok(Event::Text(e)) => {
                let text = e
                    .unescape()
                    .map_err(|_| FormatError::new("Invalid text", 0, 0))?;
                if !text.trim().is_empty() {
                    writer
                        .write_event(Event::Text(BytesText::new(&text)))
                        .map_err(|e| FormatError::new(&format!("Write error: {}", e), 0, 0))?;
                }
            }
            Ok(Event::Eof) => break,
            Ok(event) => {
                writer
                    .write_event(event)
                    .map_err(|e| FormatError::new(&format!("Write error: {}", e), 0, 0))?;
            }
            Err(e) => {
                return Err(FormatError::new(&format!("XML parse error: {}", e), 0, 0));
            }
        }
        buf.clear();
    }

    let result = writer.into_inner().into_inner();
    String::from_utf8(result).map_err(|_| FormatError::new("Invalid UTF-8 in output", 0, 0))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_format_xml_basic() {
        let input = "<root><child>text</child></root>";
        let result = format_xml(input, IndentStyle::Spaces(2)).unwrap();
        assert!(result.contains("<root>"));
        assert!(result.contains("<child>"));
        assert!(result.contains("text"));
    }

    #[test]
    fn test_format_xml_with_attributes() {
        let input = r#"<root attr="value"><child id="1"/></root>"#;
        let result = format_xml(input, IndentStyle::Spaces(2)).unwrap();
        assert!(result.contains(r#"attr="value""#));
        assert!(result.contains(r#"id="1""#));
    }

    #[test]
    fn test_format_xml_with_declaration() {
        let input = r#"<?xml version="1.0" encoding="UTF-8"?><root/>"#;
        let result = format_xml(input, IndentStyle::Spaces(2)).unwrap();
        assert!(result.contains("<?xml"));
        assert!(result.contains("<root"));
    }

    #[test]
    fn test_minify_xml() {
        let input = "<root>\n  <child>\n    text\n  </child>\n</root>";
        let result = minify_xml(input).unwrap();
        assert!(!result.contains('\n'));
        assert!(result.contains("<root><child>"));
    }

    #[test]
    fn test_roundtrip() {
        let input = r#"<root><a>1</a><b attr="x">2</b></root>"#;
        let formatted = format_xml(input, IndentStyle::Spaces(2)).unwrap();
        let minified = minify_xml(&formatted).unwrap();
        // Content should be preserved
        assert!(minified.contains("<root>"));
        assert!(minified.contains("<a>1</a>"));
        assert!(minified.contains(r#"<b attr="x">2</b>"#));
    }

    #[test]
    fn test_empty_input() {
        let result = format_xml("", IndentStyle::Spaces(2));
        assert!(result.is_err());
    }

    #[test]
    fn test_cdata() {
        let input = "<root><![CDATA[<not xml>]]></root>";
        let result = format_xml(input, IndentStyle::Spaces(2)).unwrap();
        assert!(result.contains("<![CDATA[<not xml>]]>"));
    }

    #[test]
    fn test_comments() {
        let input = "<root><!-- comment --><child/></root>";
        let result = format_xml(input, IndentStyle::Spaces(2)).unwrap();
        assert!(result.contains("<!-- comment -->"));
    }

    #[test]
    fn test_namespace_prefix() {
        let input = r#"<ns:root xmlns:ns="http://example.com"><ns:child/></ns:root>"#;
        let result = format_xml(input, IndentStyle::Spaces(2)).unwrap();
        assert!(result.contains("ns:root"));
        assert!(result.contains("ns:child"));
    }
}
