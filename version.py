Import("env")
import subprocess

git_hash=subprocess.check_output(['git', 'rev-parse', '--short=8', 'HEAD']).decode()
git_hash=git_hash.replace('\n','')

repo_clean=subprocess.check_output(['git', 'status', '-s', '-uno']).decode()

if repo_clean.count('\n') > 0:
  git_hash=git_hash+'+'

build_flag='DIGIBARO_VERSION=\\"'+git_hash+'\\"'

env.Append(CPPDEFINES=[build_flag])
