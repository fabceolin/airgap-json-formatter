//! Build script to create a minimal syntax set with only JSON syntax.
//!
//! This pre-compiles just the JSON syntax definition to reduce WASM binary size.

use std::env;
use std::fs::File;
use std::io::Write;
use std::path::Path;

use syntect::parsing::{SyntaxDefinition, SyntaxSetBuilder};

// Embedded JSON syntax definition (sublime-syntax format)
const JSON_SYNTAX_DEF: &str = r##"%YAML 1.2
---
name: JSON
file_extensions:
  - json
  - jsonl
  - sublime-settings
  - sublime-menu
  - sublime-keymap
  - sublime-mousemap
  - sublime-theme
  - sublime-build
  - sublime-project
  - sublime-completions
  - sublime-commands
  - sublime-macro
  - sublime-color-scheme
  - ipynb
  - Pipfile.lock
scope: source.json
contexts:
  main:
    - include: value
  value:
    - include: constant
    - include: number
    - include: string
    - include: array
    - include: object
    - include: comments
  constant:
    - match: \b(true|false|null)\b
      scope: constant.language.json
  number:
    - match: '-?(?:0|[1-9]\d*)(?:(?:\.\d+)?(?:[eE][+-]?\d+)?)?'
      scope: constant.numeric.json
  string:
    - match: '"'
      scope: punctuation.definition.string.begin.json
      push: inside-string
  inside-string:
    - meta_scope: string.quoted.double.json
    - match: '"'
      scope: punctuation.definition.string.end.json
      pop: true
    - include: string-escape
    - match: \n
      scope: invalid.illegal.unclosed-string.json
      pop: true
  string-escape:
    - match: '\\(?:["\\/bfnrt]|u[0-9a-fA-F]{4})'
      scope: constant.character.escape.json
    - match: \\.
      scope: invalid.illegal.unrecognized-string-escape.json
  array:
    - match: '\['
      scope: punctuation.section.sequence.begin.json
      push:
        - meta_scope: meta.sequence.json
        - match: '\]'
          scope: punctuation.section.sequence.end.json
          pop: true
        - include: value
        - match: ','
          scope: punctuation.separator.sequence.json
        - match: '[^\s\]]'
          scope: invalid.illegal.expected-sequence-separator.json
  object:
    - match: '\{'
      scope: punctuation.section.mapping.begin.json
      push:
        - meta_scope: meta.mapping.json
        - match: '\}'
          scope: punctuation.section.mapping.end.json
          pop: true
        - match: '"'
          scope: punctuation.definition.string.begin.json
          push:
            - clear_scopes: 1
            - meta_scope: meta.mapping.key.json string.quoted.double.json
            - match: '"'
              scope: punctuation.definition.string.end.json
              pop: true
            - include: string-escape
            - match: \n
              scope: invalid.illegal.unclosed-string.json
              pop: true
        - match: ':'
          scope: punctuation.separator.mapping.key-value.json
          push:
            - match: '(?=\s*\})'
              pop: true
            - include: value
            - match: '(?=,|\})'
              pop: true
            - match: '\S'
              scope: invalid.illegal.expected-mapping-value.json
        - match: ','
          scope: punctuation.separator.mapping.pair.json
        - match: '[^\s\}]'
          scope: invalid.illegal.expected-mapping-separator.json
  comments:
    - match: /\*\*(?!/)
      scope: punctuation.definition.comment.json
      push:
        - meta_scope: comment.block.documentation.json
        - match: \*/
          pop: true
    - match: /\*
      scope: punctuation.definition.comment.json
      push:
        - meta_scope: comment.block.json
        - match: \*/
          pop: true
    - match: //
      scope: punctuation.definition.comment.json
      push:
        - meta_scope: comment.line.double-slash.json
        - match: \n
          pop: true
"##;

fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();
    let dest_path = Path::new(&out_dir).join("json_syntax.bin");

    // Parse JSON syntax definition
    let json_def = SyntaxDefinition::load_from_str(JSON_SYNTAX_DEF, true, None)
        .expect("Failed to parse JSON syntax definition");

    // Build minimal SyntaxSet with only JSON
    let mut builder = SyntaxSetBuilder::new();
    builder.add_plain_text_syntax();
    builder.add(json_def);
    let minimal_ss = builder.build();

    // Dump to binary
    let binary = syntect::dumps::dump_binary(&minimal_ss);

    let mut file = File::create(&dest_path).unwrap();
    file.write_all(&binary).unwrap();

    // Also dump the theme
    let ts = syntect::highlighting::ThemeSet::load_defaults();
    let theme = &ts.themes["base16-ocean.dark"];
    let theme_binary = syntect::dumps::dump_binary(theme);

    let theme_path = Path::new(&out_dir).join("theme.bin");
    let mut theme_file = File::create(&theme_path).unwrap();
    theme_file.write_all(&theme_binary).unwrap();

    println!("cargo:rerun-if-changed=build.rs");
}
