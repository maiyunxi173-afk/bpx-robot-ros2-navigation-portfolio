# Public Release and Privacy Review

## Included intentionally

- ROS 2 source, launch files, configuration and saved map artefacts needed to review the navigation subsystem.
- A public contact email already used by the repository maintainer metadata.
- Private-network robot IP defaults used by the BPX SDK (`10.21.20.1`); these are not internet credentials.
- A labelled BPX hardware-integration clip showing the robot, live map and console view.

## Excluded from the public portfolio

- Passport, national ID, household registration, transcripts, certificates, recommendation letters and application forms.
- Employer databases, logs, customer/site data and internal deployment credentials.
- API keys, access tokens, SSH keys, cookies and environment files.
- Raw build/install/log directories and local absolute paths.
- Collaborative web-console code that is not required to review the stated personal subsystem.

## Automated and manual checks

Run the repository validator before release:

```bash
python3 scripts/validate_portfolio.py
```

It parses ROS 2 Python launch files and package XML, verifies local Markdown links, and scans text files for private-key markers and user-specific absolute paths. Binary media and vendor libraries still require manual review.

## External material and permissions

The source collaboration and third-party packages are disclosed in [source attribution](source_attribution.md). The top-level MIT licence applies only to original portfolio material; bundled components retain their own licences or vendor terms.

No unpublished employer document or dual-arm project document should be added until written public-release permission is obtained. When permission is uncertain, include a high-level personal-contribution description without proprietary code, screenshots, customer data or internal diagrams.

## Release checklist

- [ ] Run `python3 scripts/validate_portfolio.py`.
- [ ] Build on the declared ROS 2 Humble environment.
- [ ] Confirm the demo and all relative links open from the GitHub repository page.
- [ ] Inspect the commit diff for personal identifiers and credentials.
- [ ] Confirm third-party and vendor licence notices.
- [ ] Generate source archives from Git rather than copying build workspaces.
- [ ] Test each archive with `unzip -t` before uploading it to an application portal.
