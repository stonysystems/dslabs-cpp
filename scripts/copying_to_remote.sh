#!/bin/bash

# Variables
LOCAL_DIR="/Volumes/StudyMaterials/Project/janus"
ARCHIVE_NAME="janus.tar.gz"
REMOTE_USER="ubuntu"
REMOTE_HOST="130.245.173.111"
REMOTE_DIR="/home/ubuntu"

# Step 1: Compress the janus folder
echo "Compressing $LOCAL_DIR ..."
tar -czf $ARCHIVE_NAME -C "$(dirname "$LOCAL_DIR")" "$(basename "$LOCAL_DIR")"

# Step 2: Copy archive to remote server
echo "Copying $ARCHIVE_NAME to $REMOTE_USER@$REMOTE_HOST:$REMOTE_DIR ..."
scp $ARCHIVE_NAME $REMOTE_USER@$REMOTE_HOST:$REMOTE_DIR/

# Step 3: Uncompress on remote server
echo "Uncompressing on remote server ..."
ssh $REMOTE_USER@$REMOTE_HOST "cd $REMOTE_DIR && tar -xzf $ARCHIVE_NAME && rm $ARCHIVE_NAME"

# Step 4: Cleanup local archive
rm $ARCHIVE_NAME

echo "✅ Transfer complete!"
