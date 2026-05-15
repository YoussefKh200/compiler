#!/bin/bash
set -e

echo "Setting up AI optimization module..."

# Create virtual environment
python3 -m venv ai_venv
source ai_venv/bin/activate

# Install dependencies
pip install --upgrade pip
pip install torch numpy flask

# Start AI server
echo "Starting AI optimization server..."
python3 ai_module/model/optimization_ranker.py --port 5000 &

echo "AI module ready at http://localhost:5000"
