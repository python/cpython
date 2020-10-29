---
name: New extension release
about: Create an issue with a checklist for the release steps (write access required
  for the steps)
title: Release Checklist for version xx.xx.xx
labels: ''
assignees: ''

---

- [ ] Update this issue title to refer to the version of the release
- [ ] Trigger a release build on Actions by adding a new tag on branch `main` of the format `vxx.xx.xx`
- [ ] Monitor the status of the release build in the `Release` workflow in the Actions tab.
- [ ] Download the VSIX from the draft GitHub release that is created when the release build finishes.
- [ ] Log into the [Visual Studio Marketplace](https://marketplace.visualstudio.com/manage/publishers/github).
- [ ] Click the `...` menu in the CodeQL row and click **Update**.
- [ ] Drag the `.vsix` file you downloaded from the GitHub release into the Marketplace and click **Upload**.
- [ ] Publish the draft GitHub release and confirm the new release is marked as the latest release at https://github.com/github/vscode-codeql/releases.
