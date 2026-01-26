#!/bin/bash
#
# Version bump script for Airgap JSON Formatter
# Usage: ./scripts/bump_version.sh [patch|minor|major]
#
# This script updates the version in all relevant files:
#   - VERSION (root file)
#   - Cargo.toml
#   - qt/CMakeLists.txt
#   - qt/qml/Theme.qml
#   - qt/theme.h
#   - qt/main.cpp
#
# After running, commit the changes and create a git tag:
#   git add -A && git commit -m "Bump version to X.Y.Z"
#   git tag -a vX.Y.Z -m "Release vX.Y.Z"
#   git push origin main --tags
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

VERSION_FILE="$ROOT_DIR/VERSION"
CARGO_TOML="$ROOT_DIR/Cargo.toml"
CMAKE_FILE="$ROOT_DIR/qt/CMakeLists.txt"
THEME_QML="$ROOT_DIR/qt/qml/Theme.qml"
THEME_H="$ROOT_DIR/qt/theme.h"
MAIN_CPP="$ROOT_DIR/qt/main.cpp"

# Read current version
if [[ ! -f "$VERSION_FILE" ]]; then
    echo "Error: VERSION file not found at $VERSION_FILE"
    exit 1
fi

CURRENT_VERSION=$(cat "$VERSION_FILE" | tr -d '[:space:]')
echo "Current version: $CURRENT_VERSION"

# Parse version components
IFS='.' read -r MAJOR MINOR PATCH <<< "$CURRENT_VERSION"

# Determine bump type
BUMP_TYPE="${1:-patch}"

case "$BUMP_TYPE" in
    major)
        MAJOR=$((MAJOR + 1))
        MINOR=0
        PATCH=0
        ;;
    minor)
        MINOR=$((MINOR + 1))
        PATCH=0
        ;;
    patch)
        PATCH=$((PATCH + 1))
        ;;
    *)
        echo "Usage: $0 [patch|minor|major]"
        echo "  patch - Increment patch version (0.0.X)"
        echo "  minor - Increment minor version (0.X.0)"
        echo "  major - Increment major version (X.0.0)"
        exit 1
        ;;
esac

NEW_VERSION="${MAJOR}.${MINOR}.${PATCH}"
echo "New version: $NEW_VERSION"

# Update VERSION file
echo "$NEW_VERSION" > "$VERSION_FILE"
echo "✓ Updated VERSION"

# Update Cargo.toml
if [[ -f "$CARGO_TOML" ]]; then
    sed -i "s/^version = \".*\"/version = \"$NEW_VERSION\"/" "$CARGO_TOML"
    echo "✓ Updated Cargo.toml"
else
    echo "⚠ Cargo.toml not found, skipping"
fi

# Update CMakeLists.txt
if [[ -f "$CMAKE_FILE" ]]; then
    sed -i "s/project(AirgapJsonFormatter VERSION .* LANGUAGES/project(AirgapJsonFormatter VERSION $NEW_VERSION LANGUAGES/" "$CMAKE_FILE"
    echo "✓ Updated qt/CMakeLists.txt"
else
    echo "⚠ qt/CMakeLists.txt not found, skipping"
fi

# Update Theme.qml
if [[ -f "$THEME_QML" ]]; then
    sed -i "s/readonly property string appVersion: \".*\"/readonly property string appVersion: \"$NEW_VERSION\"/" "$THEME_QML"
    echo "✓ Updated qt/qml/Theme.qml"
else
    echo "⚠ qt/qml/Theme.qml not found, skipping"
fi

# Update theme.h (C++ Theme class)
if [[ -f "$THEME_H" ]]; then
    sed -i "s/return QStringLiteral(\"[0-9]*\.[0-9]*\.[0-9]*\")/return QStringLiteral(\"$NEW_VERSION\")/" "$THEME_H"
    echo "✓ Updated qt/theme.h"
else
    echo "⚠ qt/theme.h not found, skipping"
fi

# Update main.cpp (Qt application version)
if [[ -f "$MAIN_CPP" ]]; then
    sed -i "s/setApplicationVersion(\"[0-9]*\.[0-9]*\.[0-9]*\")/setApplicationVersion(\"$NEW_VERSION\")/" "$MAIN_CPP"
    echo "✓ Updated qt/main.cpp"
else
    echo "⚠ qt/main.cpp not found, skipping"
fi

echo ""
echo "=========================================="
echo "Version bumped to $NEW_VERSION"
echo "=========================================="
echo ""
echo "Next steps:"
echo "  1. Review changes: git diff"
echo "  2. Commit: git add VERSION Cargo.toml qt/CMakeLists.txt qt/qml/Theme.qml qt/theme.h qt/main.cpp"
echo "  3. Commit: git commit -m \"chore: bump version to $NEW_VERSION\""
echo "  4. Tag: git tag -a v$NEW_VERSION -m \"Release v$NEW_VERSION\""
echo "  5. Push: git push origin main --tags"
echo ""
