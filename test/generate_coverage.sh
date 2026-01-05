#!/bin/bash
# Generate code coverage reports for unit tests

set -e

echo "Running tests with coverage..."
pio test -e native

echo ""
echo "Generating coverage reports..."

# Find all .gcda files
find .pio/build/native -name "*.gcda" -type f > /dev/null 2>&1 || {
    echo "No coverage data found. Make sure tests ran successfully."
    exit 1
}

# Check if lcov is installed
if command -v lcov > /dev/null 2>&1; then
    echo "Generating lcov report..."
    
    lcov --capture \
         --directory .pio/build/native \
         --output-file coverage.info \
         --exclude '*/test/*' \
         --exclude '*/mock*' \
         --exclude '*/usr/*' \
         --exclude '*/.platformio/*'
    
    if command -v genhtml > /dev/null 2>&1; then
        genhtml coverage.info --output-directory coverage_html
        echo ""
        echo "✅ Coverage report generated: coverage_html/index.html"
        echo "   Open in browser: open coverage_html/index.html"
    else
        echo "⚠️  genhtml not found. Install with: brew install lcov (macOS) or apt install lcov (Linux)"
    fi
else
    echo "⚠️  lcov not found. Install with: brew install lcov (macOS) or apt install lcov (Linux)"
    echo ""
    echo "For now, you can view raw coverage data in .pio/build/native/"
fi

echo ""
echo "Coverage data location: .pio/build/native/"

