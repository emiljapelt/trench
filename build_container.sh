#!/bin/bash
docker build -t trench . --build-arg NODE_VERSION=$(git branch --show-current)

# Run with: docker run -t -i trench
# Publish with: 
# Attach using VSCode extension 'Dev Containers'