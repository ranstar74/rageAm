import os

class GameBuild:
    
    def __init__(self, name, code, exe, desc):
        self.name = name
        self.desc = desc
        self.code = code
        self.exe = exe

gameBuilds = [ # Keep names the same size for table alignment
    GameBuild("2699.16 Master          ", "2699_16",                "gta5.exe",               "-"),
    GameBuild("2699.16 Release (No Opt)", "2699_16_RELEASE_NO_OPT", "game_win64_release.exe", "Built from the source with Release configuration and optimizations disabled, developer use only"),
]

projectTypes = [
    "vs2022", "vs2019", "vs2017", "vs2015",
]

def inputYes(str = ""):
    return input(str).lower() == "y"

def isValidIndex(arr, index):
    return index >= 0 and index < len(arr)

# Game version
print("- rageAm solution builder\n")
print("Select game version:")
for i in range(0, len(gameBuilds)):
    gameBuild = gameBuilds[i]
    print(F"[{i}] {gameBuild.name}\t {gameBuild.desc}")
selectedBuild = int(input())
if not isValidIndex(gameBuilds, selectedBuild):
    print(F"Invalid game version '{selectedBuild}'")
    exit()

# Default configuration values
useAvx2 = True
useProfiler = False
projectType = 0 # VS22

# Advanced configuration
if inputYes("Enter advanced configuration? y/n "):
    # What CPU doesn't support AVX2 in 2022?
    useAvx2 = inputYes("Use AVX2? ")
    useProfiler = inputYes("Use profiler? ")

# Project type
print("Select project type:")
for i in range(0, len(projectTypes)):
    print(F"[{i}] {projectTypes[i]}")
projectType = int(input())
if not isValidIndex(projectTypes, projectType):
    print(F"Invalid project type '{projectType}'")
    exit()

projectTypeStr = projectTypes[projectType]
build = gameBuilds[selectedBuild]
command = F"tools\premake5.exe --file=main.lua {projectTypeStr} --exe={build.exe} --gamebuild={build.code}"
if useAvx2:
    command += " --avx2"
if useProfiler:
    command += " --easyprofiler"

print(command)
os.system(command)
