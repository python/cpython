#!/

since="90 days ago"

cd cpython

echo Copying existing repo at $since

start=$(git rev-list -n 1 --before="$since" main)
commits=$(git rev-list --reverse $commit..main)

git switch --detach $start

cd ..

rm -rf cpython-copy
cp -r cpython cpython-copy
cd cpython-copy
rm -rf .git

git init > /dev/null 2>&1
git config gc.auto 0 # Disable GC so we can run it later

git add . > /dev/null
git commit -am "init" > /dev/null

echo GCing
git gc --aggressive > /dev/null 2>&1

echo
echo Size at $since

du -sh .git

echo
echo Applying patches

patchdir=/tmp/cpython-patches
rm -rf $patchdir
mkdir $patchdir

git -C ../cpython format-patch -o $patchdir $commit..main > /dev/null

git am --3way --whitespace=nowarn --quoted-cr=nowarn --keep-cr --empty=drop $patchdir/*.patch > /dev/null

rm -rf $patchdir

echo GCing
git gc --aggressive > /dev/null 2>&1

echo
echo Size at main


du -sh .git