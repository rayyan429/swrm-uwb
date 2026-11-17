# Setup and Git workflow

Do this once before starting any of the task guides. It takes about 15 minutes.

## 1. Tools
- **VS Code** with the **PlatformIO IDE** extension (Extensions sidebar → search "PlatformIO IDE" → Install, then restart VS Code).
- **Python 3** and **Git**.
- **GitHub CLI** (`gh`), optional but handy for pull requests: `winget install GitHub.cli`, then `gh auth login`.

## 2. Get the code
```
git clone https://github.com/rayyan429/swrm-uwb.git
cd swrm-uwb
pip install -r tools/requirements.txt
```
Already cloned? Get the latest instead:
```
git checkout main
git pull
```

## 3. Check PlatformIO works
In a VS Code terminal:
```
pio --version
```
If you get **"pio is not recognized"**, PlatformIO is installed but not on your PATH. Either open the PlatformIO terminal (PlatformIO sidebar, the alien icon → Quick Access → Miscellaneous → PlatformIO Core CLI), or add it permanently by running this once in PowerShell and restarting VS Code:
```
[Environment]::SetEnvironmentVariable("Path", [Environment]::GetEnvironmentVariable("Path","User") + ";$env:USERPROFILE\.platformio\penv\Scripts", "User")
```

Every `pio` command must run from inside a project folder, `uwb_anchor/` or `uwb_tag/`. From the repo root you'll get "Not a PlatformIO project".

## 4. Unit tests (only if your task touches C++ in `lib/`)
The tests compile on your PC, so they need gcc:
```
winget install BrechtSanders.WinLibs.POSIX.UCRT
```
Restart VS Code, check `gcc --version`, then:
```
cd uwb_tag
pio test -e native
```
You should see `4 test cases: 4 succeeded`.

## 5. Git workflow
Never commit straight to `main`. Every task goes on its own branch and in through a pull request (PR).

```
git checkout main
git pull                         # start from the latest code
git checkout -b your-branch-name # make your branch (name it after the task)

# ...edit, test...

git status                       # see what changed
git add path/to/file another/file
git commit -m "Say what you changed"
git push -u origin your-branch-name   # first push; after that just: git push
```
Then open a PR: on GitHub, click the yellow **Compare & pull request** banner, or run `gh pr create --fill`. Ask someone else on the team to look it over, then **Squash and merge** and **Delete branch**. Finally:
```
git checkout main
git pull
```

Tips:
- Commit whenever something works. Push before you stop for the day, because commits only on your laptop aren't backed up.
- Write commit messages that say what changed ("Add anchor addresses from ANCHOR_ID"), not "update" or "fix".
- `git status` tells you which branch you're on and what's changed. Run it whenever you're unsure.
- If `main` moved on while you were working: `git checkout main && git pull && git checkout your-branch && git merge main`.

## 6. Where things are
- [ROADMAP.md](../ROADMAP.md): milestones, what's done, what's next.
- [FILE_LAYOUT.md](../FILE_LAYOUT.md): what each folder does.
- `lib/uwb_config/uwb_config.h`: pins, number of anchors, anchor positions, antenna delay. Shared by every board.
