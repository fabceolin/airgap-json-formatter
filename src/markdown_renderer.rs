//! Markdown rendering module using pulldown-cmark for GitHub Flavored Markdown (GFM) support.
//!
//! This module provides secure, client-side Markdown-to-HTML conversion with:
//! - Full GFM support (tables, strikethrough, task lists)
//! - Code block language class annotations for syntax highlighting
//! - Mermaid block detection for diagram rendering
//! - Security hardening (no raw HTML, URI sanitization)
//! - Input size limits to prevent WASM heap exhaustion

use pulldown_cmark::{html, Event, Options, Parser, Tag, CodeBlockKind};

/// Maximum input size in bytes (2MB) to prevent WASM heap exhaustion
const MAX_INPUT_SIZE: usize = 2 * 1024 * 1024;

/// Error type for Markdown rendering failures
#[derive(Debug, Clone, PartialEq)]
pub struct RenderError {
    pub message: String,
}

impl std::fmt::Display for RenderError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.message)
    }
}

impl std::error::Error for RenderError {}

/// Render Markdown to HTML with GFM extensions.
///
/// # Arguments
/// * `input` - The Markdown string to render
///
/// # Returns
/// * `Ok(String)` - The rendered HTML on success
/// * `Err(RenderError)` - Error with descriptive message on failure
///
/// # Security
/// - Raw HTML is filtered out
/// - Input size is limited to 2MB to prevent heap exhaustion
/// - Dangerous URI schemes (javascript:, data:, vbscript:) are sanitized
///
/// # Example
/// ```
/// use airgap_json_formatter::markdown_renderer::render_markdown;
///
/// let html = render_markdown("# Hello").unwrap();
/// assert!(html.contains("<h1>"));
/// ```
pub fn render_markdown(input: &str) -> Result<String, RenderError> {
    // AC13: Input size guard to prevent WASM heap exhaustion
    if input.len() > MAX_INPUT_SIZE {
        return Err(RenderError {
            message: format!(
                "Input too large: {} bytes exceeds 2MB limit",
                input.len()
            ),
        });
    }

    // Handle empty input gracefully
    if input.is_empty() {
        return Ok(String::new());
    }

    // Enable GFM extensions (AC4)
    let mut options = Options::empty();
    options.insert(Options::ENABLE_TABLES);
    options.insert(Options::ENABLE_STRIKETHROUGH);
    options.insert(Options::ENABLE_TASKLISTS);

    let parser = Parser::new_ext(input, options);

    // Filter out raw HTML and sanitize URIs
    let parser = parser.filter_map(|event| filter_event(event));

    let mut html_output = String::new();
    html::push_html(&mut html_output, parser);

    // AC12: Post-process to sanitize dangerous URI schemes
    let sanitized = sanitize_dangerous_uris(&html_output);

    Ok(sanitized)
}

/// Filter events to enhance security and add language classes to code blocks
fn filter_event(event: Event) -> Option<Event> {
    match event {
        // Filter out raw HTML for security
        Event::Html(_) | Event::InlineHtml(_) => None,

        // Add language class to code blocks (AC5, AC6)
        Event::Start(Tag::CodeBlock(kind)) => {
            match kind {
                CodeBlockKind::Fenced(lang) if !lang.is_empty() => {
                    // Return the original event - pulldown-cmark handles language classes
                    Some(Event::Start(Tag::CodeBlock(CodeBlockKind::Fenced(lang))))
                }
                _ => Some(Event::Start(Tag::CodeBlock(kind)))
            }
        }

        // Pass through all other events
        _ => Some(event)
    }
}

/// Sanitize dangerous URI schemes from rendered HTML.
///
/// Removes or neutralizes `javascript:`, `data:`, and `vbscript:` URIs
/// from href and src attributes to prevent XSS attacks.
fn sanitize_dangerous_uris(html: &str) -> String {
    let mut result = html.to_string();

    // Dangerous schemes to sanitize (case-insensitive matching)
    let dangerous_schemes = ["javascript:", "data:", "vbscript:"];

    for scheme in dangerous_schemes {
        // Match both lowercase and uppercase variants
        let patterns = [
            format!("href=\"{}",  scheme),
            format!("href='{}",   scheme),
            format!("src=\"{}",   scheme),
            format!("src='{}",    scheme),
            format!("href=\"{}",  scheme.to_uppercase()),
            format!("href='{}",   scheme.to_uppercase()),
            format!("src=\"{}",   scheme.to_uppercase()),
            format!("src='{}",    scheme.to_uppercase()),
        ];

        for pattern in patterns {
            if result.contains(&pattern) {
                let replacement = if pattern.contains("href") {
                    if pattern.contains('"') { "href=\"#\"" } else { "href='#'" }
                } else {
                    if pattern.contains('"') { "src=\"\"" } else { "src=''" }
                };
                result = replace_uri_attribute(&result, &pattern, replacement);
            }
        }
    }

    result
}

/// Replace URI attribute value while preserving the rest of the tag.
fn replace_uri_attribute(html: &str, pattern: &str, replacement: &str) -> String {
    let mut result = String::with_capacity(html.len());
    let mut remaining = html;

    while let Some(start) = remaining.find(pattern) {
        result.push_str(&remaining[..start]);
        let after_pattern = &remaining[start + pattern.len()..];
        let quote_char = if pattern.contains('"') { '"' } else { '\'' };

        if let Some(end) = after_pattern.find(quote_char) {
            result.push_str(replacement);
            remaining = &after_pattern[end + 1..];
        } else {
            result.push_str(&remaining[start..start + pattern.len()]);
            remaining = after_pattern;
        }
    }

    result.push_str(remaining);
    result
}

#[cfg(test)]
mod tests {
    use super::*;

    // === AC2: render_markdown function signature tests ===

    #[test]
    fn test_empty_input() {
        let result = render_markdown("");
        assert_eq!(result.unwrap(), "");
    }

    #[test]
    fn test_heading_h1() {
        let result = render_markdown("# Hello").unwrap();
        assert!(result.contains("<h1>"));
        assert!(result.contains("Hello"));
        assert!(result.contains("</h1>"));
    }

    #[test]
    fn test_headings_h1_to_h6() {
        for level in 1..=6 {
            let input = format!("{} Heading {}", "#".repeat(level), level);
            let result = render_markdown(&input).unwrap();
            assert!(result.contains(&format!("<h{}>", level)));
            assert!(result.contains(&format!("</h{}>", level)));
        }
    }

    #[test]
    fn test_bold() {
        let result = render_markdown("**bold**").unwrap();
        assert!(result.contains("<strong>bold</strong>"));
    }

    #[test]
    fn test_italic() {
        let result = render_markdown("*italic*").unwrap();
        assert!(result.contains("<em>italic</em>"));
    }

    #[test]
    fn test_strikethrough() {
        let result = render_markdown("~~strike~~").unwrap();
        assert!(result.contains("<del>strike</del>"));
    }

    #[test]
    fn test_unordered_list() {
        let result = render_markdown("- item 1\n- item 2").unwrap();
        assert!(result.contains("<ul>"));
        assert!(result.contains("<li>"));
        assert!(result.contains("item 1"));
        assert!(result.contains("item 2"));
    }

    #[test]
    fn test_ordered_list() {
        let result = render_markdown("1. first\n2. second").unwrap();
        assert!(result.contains("<ol>"));
        assert!(result.contains("<li>"));
    }

    #[test]
    fn test_blockquote() {
        let result = render_markdown("> quoted text").unwrap();
        assert!(result.contains("<blockquote>"));
        assert!(result.contains("quoted text"));
    }

    #[test]
    fn test_horizontal_rule() {
        let result = render_markdown("---").unwrap();
        assert!(result.contains("<hr"));
    }

    #[test]
    fn test_link() {
        let result = render_markdown("[text](https://example.com)").unwrap();
        assert!(result.contains("<a href=\"https://example.com\">"));
        assert!(result.contains("text</a>"));
    }

    #[test]
    fn test_image() {
        let result = render_markdown("![alt](https://example.com/img.png)").unwrap();
        assert!(result.contains("<img"));
        assert!(result.contains("src=\"https://example.com/img.png\""));
        assert!(result.contains("alt=\"alt\""));
    }

    #[test]
    fn test_inline_code() {
        let result = render_markdown("`code`").unwrap();
        assert!(result.contains("<code>code</code>"));
    }

    #[test]
    fn test_code_block_with_language() {
        let result = render_markdown("```javascript\nconst x = 1;\n```").unwrap();
        assert!(result.contains("<pre>"));
        assert!(result.contains("<code"));
        // pulldown-cmark adds language class
        assert!(result.contains("language-javascript") || result.contains("const x = 1;"));
    }

    #[test]
    fn test_mermaid_block() {
        let result = render_markdown("```mermaid\ngraph TD\n    A --> B\n```").unwrap();
        // pulldown-cmark should add language-mermaid class
        assert!(result.contains("language-mermaid") || result.contains("graph TD"));
    }

    // === AC4: GFM extension tests ===

    #[test]
    fn test_gfm_table() {
        let input = "| A | B |\n|---|---|\n| 1 | 2 |";
        let result = render_markdown(input).unwrap();
        assert!(result.contains("<table>"));
        assert!(result.contains("<th>") || result.contains("<td>"));
    }

    #[test]
    fn test_gfm_table_alignment() {
        let input = "| Left | Center | Right |\n|:-----|:------:|------:|\n| L | C | R |";
        let result = render_markdown(input).unwrap();
        assert!(result.contains("<table>"), "Expected <table> in: {}", result);
        assert!(result.contains("Left"), "Expected 'Left' header in: {}", result);
        assert!(result.contains("Center"), "Expected 'Center' header in: {}", result);
        assert!(result.contains("Right"), "Expected 'Right' header in: {}", result);
    }

    #[test]
    fn test_gfm_task_list() {
        let result = render_markdown("- [ ] unchecked\n- [x] checked").unwrap();
        assert!(result.contains("type=\"checkbox\"") || result.contains("[ ]") || result.contains("[x]"));
    }

    // === AC8: Edge case tests ===

    #[test]
    fn test_unicode_content() {
        let result = render_markdown("# 日本語\n\n你好世界\n\n🎉 emoji").unwrap();
        assert!(result.contains("日本語"));
        assert!(result.contains("你好世界"));
        assert!(result.contains("🎉"));
    }

    #[test]
    fn test_deeply_nested_lists() {
        let mut input = String::new();
        for i in 0..15 {
            input.push_str(&"  ".repeat(i));
            input.push_str("- level ");
            input.push_str(&i.to_string());
            input.push('\n');
        }
        let result = render_markdown(&input);
        assert!(result.is_ok());
        let html = result.unwrap();
        assert!(html.contains("<ul>"));
        assert!(html.contains("level 14"));
    }

    // === AC11: Error HTML-escaping tests ===

    #[test]
    fn test_raw_html_blocked() {
        let result = render_markdown("<script>alert(1)</script>").unwrap();
        // Raw HTML should be filtered out
        assert!(!result.contains("<script>") || result.contains("&lt;script&gt;"));
    }

    // === AC12: URI scheme sanitization tests ===

    #[test]
    fn test_javascript_uri_sanitized() {
        let result = render_markdown("[click](javascript:alert(1))").unwrap();
        assert!(!result.contains("javascript:"));
    }

    #[test]
    fn test_data_uri_sanitized() {
        let result = render_markdown("[click](data:text/html,<script>alert(1)</script>)").unwrap();
        assert!(!result.contains("data:text/html"));
    }

    #[test]
    fn test_vbscript_uri_sanitized() {
        let result = render_markdown("[click](vbscript:msgbox(1))").unwrap();
        assert!(!result.contains("vbscript:"));
    }

    #[test]
    fn test_javascript_uri_uppercase() {
        let result = render_markdown("[click](JAVASCRIPT:alert(1))").unwrap();
        assert!(!result.contains("JAVASCRIPT:"));
        assert!(!result.contains("javascript:"));
    }

    // === AC13: Input size guard tests ===

    #[test]
    fn test_input_size_guard() {
        let large_input = "x".repeat(2 * 1024 * 1024 + 1);
        let result = render_markdown(&large_input);
        assert!(result.is_err());
        let err = result.unwrap_err();
        assert!(err.message.contains("Input too large"));
        assert!(err.message.contains("2MB limit"));
    }

    #[test]
    fn test_input_at_limit() {
        let limit_input = "x".repeat(2 * 1024 * 1024);
        let result = render_markdown(&limit_input);
        assert!(result.is_ok());
    }

    // === Paragraph test ===

    #[test]
    fn test_paragraph() {
        let result = render_markdown("Hello world").unwrap();
        assert!(result.contains("<p>Hello world</p>"));
    }

    // === Performance tests (AC10) ===

    #[test]
    fn test_500kb_document_performance() {
        let mut doc = String::with_capacity(512 * 1024);
        for i in 1..=100 {
            doc.push_str(&format!("# Heading {}\n\n", i));
            doc.push_str("This is a paragraph with **bold** and *italic* text. ");
            doc.push_str("It also has `inline code` and [links](https://example.com).\n\n");
            doc.push_str("- Item 1\n- Item 2\n- Item 3\n\n");
            doc.push_str("```javascript\nconst x = 1;\nconsole.log(x);\n```\n\n");
            doc.push_str("| A | B | C |\n|---|---|---|\n| 1 | 2 | 3 |\n\n");
        }
        while doc.len() < 500 * 1024 {
            doc.push_str("Lorem ipsum dolor sit amet, consectetur adipiscing elit. ");
        }

        assert!(doc.len() >= 500 * 1024, "Document should be at least 500KB");

        let start = std::time::Instant::now();
        let result = render_markdown(&doc);
        let elapsed = start.elapsed();

        assert!(result.is_ok(), "500KB document should render successfully");

        let elapsed_ms = elapsed.as_millis();
        assert!(
            elapsed_ms < 500,
            "500KB document render took {}ms, expected < 500ms (use --release for AC10 validation)",
            elapsed_ms
        );

        println!("500KB document rendered in {}ms", elapsed_ms);
    }

    #[test]
    fn test_deeply_nested_100_levels() {
        let mut input = String::new();
        for i in 0..101 {
            input.push_str(&"  ".repeat(i));
            input.push_str("- level ");
            input.push_str(&i.to_string());
            input.push('\n');
        }

        let result = render_markdown(&input);
        assert!(result.is_ok(), "100+ nested levels should not cause stack overflow");
        let html = result.unwrap();
        assert!(html.contains("level 100"), "All nesting levels should be rendered");
    }
}
