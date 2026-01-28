#!/usr/bin/env node
/**
 * WCAG Contrast Ratio Verification Script (Story 10.7)
 *
 * Verifies all foreground/background color pairs meet WCAG AA requirements (4.5:1 minimum)
 *
 * Usage: node scripts/verify-contrast.js
 */

// Color definitions from Theme.qml
const colors = {
    dark: {
        background: '#1e1e1e',
        previewBackground: '#1e1e1e',
        previewCodeBg: '#252526',
        syntaxMdHeading: '#569cd6',
        syntaxMdStrike: '#9d9d9d',
        syntaxMdLink: '#4ec9b0',
        syntaxMdUrl: '#ce9178',
        syntaxMdBlockquote: '#73a561',
        syntaxMdListMarker: '#d4d4d4',
        previewText: '#d4d4d4',
        previewHeading: '#569cd6',
        previewLink: '#4ec9b0',
        previewCodeText: '#d4d4d4',
        mermaidPrimaryColor: '#1a365d',
        mermaidPrimaryTextColor: '#d4d4d4'
    },
    light: {
        background: '#f5f5f5',
        previewBackground: '#f5f5f5',
        previewCodeBg: '#f6f8fa',
        syntaxMdHeading: '#0066cc',
        syntaxMdStrike: '#57606a',
        syntaxMdLink: '#1a7f37',
        syntaxMdUrl: '#6f42c1',
        syntaxMdBlockquote: '#57606a',
        syntaxMdListMarker: '#24292e',
        previewText: '#24292e',
        previewHeading: '#0066cc',
        previewLink: '#0366d6',
        previewCodeText: '#24292e',
        mermaidPrimaryColor: '#dbeafe',
        mermaidPrimaryTextColor: '#1e3a5f'
    }
};

// Parse hex color to RGB
function hexToRgb(hex) {
    const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
    } : null;
}

// Calculate relative luminance per WCAG 2.1
// https://www.w3.org/WAI/GL/wiki/Relative_luminance
function relativeLuminance(rgb) {
    const [r, g, b] = [rgb.r, rgb.g, rgb.b].map(channel => {
        const c = channel / 255;
        return c <= 0.03928 ? c / 12.92 : Math.pow((c + 0.055) / 1.055, 2.4);
    });
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

// Calculate contrast ratio per WCAG 2.1
// https://www.w3.org/WAI/GL/wiki/Contrast_ratio
function contrastRatio(fg, bg) {
    const l1 = relativeLuminance(hexToRgb(fg));
    const l2 = relativeLuminance(hexToRgb(bg));
    const lighter = Math.max(l1, l2);
    const darker = Math.min(l1, l2);
    return (lighter + 0.05) / (darker + 0.05);
}

// Color pairs to verify
const colorPairs = [
    // Syntax colors on background
    { fg: 'syntaxMdHeading', bg: 'background', label: 'syntaxMdHeading on background' },
    { fg: 'syntaxMdStrike', bg: 'background', label: 'syntaxMdStrike on background' },
    { fg: 'syntaxMdLink', bg: 'background', label: 'syntaxMdLink on background' },
    { fg: 'syntaxMdUrl', bg: 'background', label: 'syntaxMdUrl on background' },
    { fg: 'syntaxMdBlockquote', bg: 'background', label: 'syntaxMdBlockquote on background' },
    { fg: 'syntaxMdListMarker', bg: 'background', label: 'syntaxMdListMarker on background' },
    // Preview colors
    { fg: 'previewText', bg: 'previewBackground', label: 'previewText on previewBackground' },
    { fg: 'previewHeading', bg: 'previewBackground', label: 'previewHeading on previewBackground' },
    { fg: 'previewLink', bg: 'previewBackground', label: 'previewLink on previewBackground' },
    { fg: 'previewCodeText', bg: 'previewCodeBg', label: 'previewCodeText on previewCodeBg' },
    // Mermaid colors
    { fg: 'mermaidPrimaryTextColor', bg: 'mermaidPrimaryColor', label: 'mermaid primaryTextColor on primaryColor' }
];

const MIN_RATIO = 4.5;
let allPassed = true;
let results = [];

console.log('WCAG AA Contrast Ratio Verification (Story 10.7)');
console.log('='.repeat(60));
console.log(`Minimum required ratio: ${MIN_RATIO}:1\n`);

for (const mode of ['dark', 'light']) {
    console.log(`\n${mode.toUpperCase()} THEME:`);
    console.log('-'.repeat(60));

    for (const pair of colorPairs) {
        const fgColor = colors[mode][pair.fg];
        const bgColor = colors[mode][pair.bg];
        const ratio = contrastRatio(fgColor, bgColor);
        const passed = ratio >= MIN_RATIO;
        const status = passed ? '✅ PASS' : '❌ FAIL';
        const level = ratio >= 7 ? 'AAA' : ratio >= 4.5 ? 'AA' : 'FAIL';

        results.push({ mode, pair: pair.label, ratio, passed, level });

        if (!passed) {
            allPassed = false;
        }

        console.log(`${status} | ${pair.label}`);
        console.log(`       ${fgColor} on ${bgColor} = ${ratio.toFixed(2)}:1 (${level})`);
    }
}

console.log('\n' + '='.repeat(60));
console.log('SUMMARY');
console.log('='.repeat(60));

const passCount = results.filter(r => r.passed).length;
const failCount = results.filter(r => !r.passed).length;

console.log(`Total pairs tested: ${results.length}`);
console.log(`Passed: ${passCount}`);
console.log(`Failed: ${failCount}`);
console.log(`\nOverall: ${allPassed ? '✅ ALL PASS - WCAG AA compliant' : '❌ SOME FAILED - Fix required'}`);

process.exit(allPassed ? 0 : 1);
