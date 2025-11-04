#!/bin/bash
#
# Script to fix the branch issue where ctrlsyn pushed to main instead of anas
#
# This script will:
# 1. Reset the main branch to the commit before ctrlsyn's push
# 2. Add ctrlsyn's commit to the anas branch
#
# IMPORTANT: Run this script only if you have push access to both main and anas branches
#

set -e  # Exit on error

echo "=========================================="
echo "Branch Fix Script"
echo "=========================================="
echo ""
echo "This script will fix the issue where ctrlsyn pushed to main instead of anas"
echo ""
echo "Current situation:"
echo "  - Main branch points to: 43517545 (Sept 27, 2025) - 'New base code. non blocking'"
echo "  - This commit should be on the anas branch instead"
echo ""
echo "What this script will do:"
echo "  1. Reset main branch to 5f721b158ce2aeb1097e407ae0c92e836baefc59 (Sept 11 merge commit)"
echo "  2. Add commit 43517545 to the anas branch"
echo ""
read -p "Do you want to proceed? (yes/no): " confirm

if [ "$confirm" != "yes" ]; then
    echo "Aborted."
    exit 1
fi

echo ""
echo "Step 1: Fetching latest changes..."
git fetch origin

echo ""
echo "Step 2: Checking out anas branch..."
git checkout anas || git checkout -b anas origin/anas

echo ""
echo "Step 3: Cherry-picking ctrlsyn's commit to anas branch..."
git cherry-pick 43517545f04fd9e82253652964e86a1bb7974b9e

echo ""
echo "Step 4: Pushing updated anas branch..."
git push origin anas

echo ""
echo "Step 5: Resetting main branch to the correct commit..."
git checkout main
git reset --hard 5f721b158ce2aeb1097e407ae0c92e836baefc59

echo ""
echo "Step 6: Force pushing main branch (this requires force push permissions)..."
read -p "Are you sure you want to force push to main? This will rewrite history! (yes/no): " confirm_force

if [ "$confirm_force" != "yes" ]; then
    echo "Aborted force push to main. Main branch is now at the correct commit locally."
    echo "To complete the fix, you need to force push: git push --force origin main"
    exit 1
fi

git push --force origin main

echo ""
echo "=========================================="
echo "Done!"
echo "=========================================="
echo ""
echo "Summary of changes:"
echo "  - Main branch reset to: 5f721b158 (Sept 11, 2025)"
echo "  - Anas branch now includes: 43517545 (Sept 27, 2025)"
echo ""
echo "Next steps:"
echo "  1. Verify the changes on GitHub"
echo "  2. Create a PR from anas to main for proper review"
echo "  3. Consider enabling branch protection on main to prevent direct pushes"
