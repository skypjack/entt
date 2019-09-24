#!/bin/sh

# only argument should be the version to upgrade to
if [ $# != 1 ]
then
  echo "Expected a version tag like v2.7.1"
  exit 1
fi

VERSION="$1"
URL="https://github.com/skypjack/entt/archive/$VERSION.tar.gz"
FORMULA="entt.rb"

echo "Updating homebrew package to $VERSION"

echo "Cloning..."
git clone https://github.com/skypjack/homebrew-entt.git
if [ $? != 0 ]
then
  exit 1
fi
cd homebrew-entt

# download the repo at the version
# exit with error messages if curl fails
echo "Curling..."
curl "$URL" --location --fail --silent --show-error --output archive.tar.gz
if [ $? != 0 ]
then
  exit 1
fi

# compute sha256 hash
echo "Hashing..."
HASH="$(openssl sha256 archive.tar.gz | cut -d " " -f 2)"

# delete the archive
rm archive.tar.gz

echo "Sedding..."

# change the url in the formula file
# the slashes in the URL must be escaped
ESCAPED_URL="$(echo "$URL" | sed -e 's/[\/&]/\\&/g')"
sed -i -e '/url/s/".*"/"'$ESCAPED_URL'"/' $FORMULA

# change the hash in the formula file
sed -i -e '/sha256/s/".*"/"'$HASH'"/' $FORMULA

# delete temporary file created by sed
rm -rf "$FORMULA-e"

# update remote repo
echo "Gitting..."
git add entt.rb
git commit -m "Update to $VERSION"
git push origin master

# out of homebrew-entt dir
cd ..
