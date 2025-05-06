# How to release a new version of VITESS

## Version Bump

### Change version in files:

- **README.md**: Image link for the badge, Version numbers in the download section
- **.gitlab-ci.yml**: VITESS_VERSION
- **SRC/Makefile**: VITESS_VERSION
- **SRC/vitess.mak**: VERSION_MAJOR, VERSION_MINOR
- **Vitess**: set versionNumber

### Add windows installer file:
- Copy an older file to TOOLS/Win10Vitess<Version>.iss
- Adjust the release year
- Adjust the version number in title, directories, and desktop link

### Additional tasks:

- Add Release Notes to RelNotes/
- Make sure the repositories vitess-<OS>-data have the tag vitess<VERSION>

## Merging

- Merge develop into master
- Create the tag
- Rebase develop onto master or merge master back into develop (so tools get the correct version from git)

## Gitlab release

- Add links ("Windows Binaries" etc.) to the artifacts from the "package" job of the release tag.
- Add a link "All Compiled Binaries" to the download folder of the artifacts
- Add the release notes
