#!/bin/bash
#
# Install git hooks from .githooks/ directory
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
GIT_HOOKS_DIR="$(git rev-parse --git-path hooks)"

echo "Installing git hooks..."

# Copy all executable files from .githooks to .git/hooks
for hook in "$SCRIPT_DIR"/*; do
    # Skip install.sh itself
    if [ "$(basename "$hook")" = "install.sh" ]; then
        continue
    fi

    # Skip README
    if [ "$(basename "$hook")" = "README.md" ]; then
        continue
    fi

    if [ -f "$hook" ] && [ -x "$hook" ]; then
        hook_name=$(basename "$hook")
        echo "  Installing $hook_name..."
        cp "$hook" "$GIT_HOOKS_DIR/$hook_name"
        chmod +x "$GIT_HOOKS_DIR/$hook_name"
    fi
done

echo "✓ Git hooks installed successfully"
echo ""
echo "Installed hooks:"
ls -la "$GIT_HOOKS_DIR" | grep -E "^-rwx" | awk '{print "  - " $9}'
