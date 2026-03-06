# CI/CD

## Workflows

| Workflow | Trigger | Action |
|---|---|---|
| `ci.yml` | Push / PR on `main`, `develop` | Build + test |
| `release-drafter.yml` | PR merged to `main` | Update release draft notes |
| `release.yml` | `git push tag v*.*.*` | Build, package, publish release |

---

## Release a New Version

```bash
git tag v0.2.0
git push origin v0.2.0
```

GitHub Actions will:

1. Build the binary in Release mode
2. Package it as `defender-v0.2.0-linux-x86_64.tar.gz`
3. Publish it to the [Releases](https://github.com/chuipagro/defender/releases) page

---

## Conventional Commits → Auto-labels

PR titles follow [Conventional Commits](https://www.conventionalcommits.org/) and are **auto-labeled**:

| Prefix | Label | Release section |
|---|---|---|
| `feat:` | `feature` | 🚀 New Features |
| `fix:` | `bug` | 🐛 Bug Fixes |
| `perf:` | `performance` | ⚡ Performance |
| `refactor:` | `enhancement` | ✨ Enhancements |
| `docs:` | `docs` | 📖 Documentation |
| `chore:` / `ci:` / `test:` | `chore` | 🔧 Chores |
| `breaking:` | `breaking` | 💥 Breaking Changes |

!!! tip "Auto release notes"
    The `release-drafter.yml` workflow automatically maintains a draft release with categorized changelog entries based on merged PR titles. No manual changelog editing needed.
