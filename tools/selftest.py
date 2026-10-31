"""selftest.py: confirm the compiler works on this machine before a long run.
Compiles a unit whose functions all match on the build machine and expects the same result."""
import os, sys, subprocess, time
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
UNIT = os.path.join(ROOT, 'src', 'enginex', 'EXTrigger.cpp')
t0 = time.time()
r = subprocess.run([sys.executable, os.path.join(HERE, 'check.py'), UNIT], capture_output=True, text=True)
out = (r.stdout + r.stderr).strip()
print(out[-2000:])
last = out.splitlines()[-1] if out else ''
ok = r.returncode == 0 and ' match ' in last and last.split()[0].split('/')[0] == last.split()[0].split('/')[1]
print(f'compile + compare took {time.time() - t0:.1f} s')
print('SELF-TEST PASSED' if ok else 'SELF-TEST FAILED')
sys.exit(0 if ok else 1)
