#!/usr/bin/env python3
import subprocess, math, sys, os, re

BIN_DIR = "bin"
N = 100000
WORKERS = 4
TOL = 1e-2   # 1% de error permitido

def run_and_check(exe):
    if "secuencial" in exe:
        args = [exe, str(N)]
    else:
        args = [exe, str(N), str(WORKERS)]

    try:
        out = subprocess.check_output(args, text=True).strip()
        # Buscar el número que aparece después de "PI"
        match = re.search(r"PI.*?:\s*([0-9]+\.[0-9]+)", out, re.IGNORECASE)
        if not match:
            return None, f"no se encontró número en la salida:\n{out}"
        pi_est = float(match.group(1))
        err = abs(pi_est - math.pi) / math.pi
        return pi_est, err
    except Exception as e:
        return None, str(e)

def main():
    if not os.path.isdir(BIN_DIR):
        print(f"No existe el directorio {BIN_DIR}. Primero compila con 'make all'")
        sys.exit(1)

    fails = []
    for exe in sorted(os.listdir(BIN_DIR)):
        path = os.path.join(BIN_DIR, exe)
        if not os.access(path, os.X_OK):
            continue
        pi_est, err = run_and_check(path)
        if pi_est is None:
            print(f"ERROR {exe}: al ejecutar ({err})")
            fails.append(exe)
            continue
        if err < TOL:
            print(f"OK {exe}: pi≈{pi_est:.6f}, error={err:.2%}")
        else:
            print(f"FAIL {exe}: pi≈{pi_est:.6f}, error={err:.2%} > {TOL:.2%}")
            fails.append(exe)

    if fails:
        print(f"\nResumen: {len(fails)} fallaron → {', '.join(fails)}")
        sys.exit(1)
    else:
        print("\nTodos los ejecutables pasaron la verificación")
        sys.exit(0)

if __name__ == "__main__":
    main()
