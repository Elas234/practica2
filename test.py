import subprocess

EXEC = "./practica2SG"
MAP_DIR = "mapas"

tests = [

    # Tests nivel 1
    ("mapa30.map", [0, 0, 17, 5, 0, 17, 17, 0, 3, 3, 0], None),
    ("mapa30.map", [0, 0, 24, 7, 2, 17, 17, 0, 3, 3, 0], None),
    ("mapa30.map", [0, 0, 24, 10, 2, 17, 17, 0, 3, 3, 0], None),
    ("mapa30.map", [0, 0, 16, 9, 2, 16, 14, 6, 3, 3, 0], None),
    ("mapa75.map", [0, 0, 10, 18, 2, 42, 30, 6, 3, 3, 0], None),
    ("gemini2.map", [0, 0, 3, 10, 2, 3, 13, 6, 3, 3, 0], None),
    ("paldea25.map", [1, 0, 76, 15, 6, 22, 68, 2, 3, 3, 0], None),
    ("chess.map", [1, 0, 45, 4, 0, 45, 14, 0, 3, 3, 0], None),
    ("luminalia25.map", [1, 0, 28, 14, 3, 27, 84, 5, 3, 3, 0], None),
    ("bosque_prohibido.map", [1, 0, 24, 5, 4, 12, 7, 4, 3, 3, 0], None),
    ("islas25.map", [1, 0, 11, 3, 2, 12, 3, 2, 3, 3, 0], None),

    # Tests nivel 2
    ("mapa30.map", [1, 2, 11, 4, 4, 26, 26, 0, 3, 4, 0], 2989),
    ("mapa30.map", [1, 2, 24, 9, 4, 26, 26, 0, 20, 9, 0], 2979),
    ("mapa30.map", [1, 2, 24, 13, 0, 26, 26, 0, 25, 17, 0], 2983),
    ("mapa50.map", [1, 2, 40, 16, 6, 46, 27, 2, 27, 3, 0], 2737),
    ("luminalia25.map", [1, 2, 32, 74, 6, 5, 96, 0, 32, 75, 0], 2962),
    ("luminalia25.map", [1, 2, 80, 58, 6, 5, 96, 0, 20, 48, 0], 2926),
    ("paldea25.map", [1, 2, 91, 45, 2, 96, 96, 0, 30, 57, 0], 2913),
    ("paldea25.map", [1, 2, 53, 85, 6, 96, 96, 0, 49, 4, 0], 2757),
    ("chess.map", [1, 2, 25, 24, 0, 46, 46, 3, 24, 24, 0], 2951),
    ("chess.map", [1, 2, 27, 27, 0, 46, 46, 3, 26, 27, 0], 2954),
    ("luminalia25.map", [1, 2, 59, 89, 2, 5, 96, 0, 29, 15, 0], 2914),
    ("scape25.map", [1, 2, 7, 8, 0, 15, 8, 5, 22, 8, 0], 2949),
    ("scape25.map", [1, 2, 7, 8, 0, 15, 8, 5, 22, 15, 0], 2955),
    ("scape25.map", [1, 2, 7, 8, 0, 15, 8, 5, 22, 21, 0], 2961),
    ("scape25.map", [1, 2, 7, 14, 0, 15, 8, 5, 22, 8, 0], 2939),
    ("scape25.map", [1, 2, 7, 14, 0, 15, 8, 5, 22, 15, 0], 2945),
    ("scape25.map", [1, 2, 7, 14, 0, 15, 8, 5, 22, 21, 0], 2951),
    ("scape25.map", [1, 2, 7, 21, 0, 15, 8, 5, 22, 8, 0], 2929),
    ("scape25.map", [1, 2, 7, 21, 0, 15, 8, 5, 22, 15, 0], 2935),
    ("scape25.map", [1, 2, 7, 21, 0, 15, 8, 5, 22, 21, 0], 2941),
    ("scape25.map", [1, 2, 7, 8, 2, 15, 8, 5, 22, 8, 0], 2951),

    # Tests nivel 3
    ("mapa30.map", [1, 3, 7, 7, 2, 11, 6, 4, 12, 5, 0], 2975),
    ("mapa30.map", [1, 3, 5, 5, 2, 10, 10, 4, 12, 5, 0], 2592),
    ("mapa30.map", [1, 3, 26, 26, 0, 11, 4, 4, 3, 4, 0], 2979),
    ("mapa30.map", [1, 3, 26, 26, 0, 24, 9, 4, 20, 9, 0], 2961),
    ("mapa50.map", [1, 3, 46, 27, 0, 20, 8, 2, 18, 12, 0], 2642),
    ("mapa50.map", [1, 3, 46, 27, 2, 40, 16, 6, 27, 3, 0], 2621),
    ("luminalia25.map", [1, 3, 5, 96, 0, 32, 74, 6, 32, 75, 0], 2828),
    ("luminalia25.map", [1, 3, 5, 96, 0, 80, 58, 6, 20, 48, 0], 2782),
    ("paldea25.map", [1, 3, 96, 96, 0, 91, 45, 2, 30, 57, 0], 2846),
    ("paldea25.map", [1, 3, 96, 96, 0, 53, 85, 6, 49, 4, 0], 2571),
    ("chess.map", [1, 3, 46, 46, 3, 25, 24, 0, 24, 24, 0], 2925),
    ("chess.map", [1, 3, 46, 46, 3, 27, 27, 0, 26, 27, 0], 2934),
    ("luminalia25.map", [1, 3, 5, 96, 0, 59, 89, 2, 29, 15, 0], 2659),
    ("scape25.map", [1, 3, 15, 8, 5, 22, 21, 4, 7, 8, 0], 2816),
    ("scape25.map", [1, 3, 15, 8, 5, 22, 21, 4, 7, 14, 0], 2694),
    ("scape25.map", [1, 3, 15, 8, 5, 22, 21, 4, 7, 21, 0], 2670),
    ("scape25.map", [1, 3, 15, 8, 5, 22, 21, 6, 7, 21, 0], 2672),
    ("scape25.map", [1, 3, 15, 8, 5, 22, 15, 4, 7, 8, 0], 2670),
    ("scape25.map", [1, 3, 15, 8, 5, 22, 15, 4, 7, 14, 0], 2758),
    ("scape25.map", [1, 3, 15, 8, 5, 22, 15, 4, 7, 21, 0], 2753),
    ("scape25.map", [1, 3, 15, 8, 5, 22, 8, 4, 7, 8, 0], 2643),
    ("scape25.map", [1, 3, 15, 8, 5, 22, 8, 4, 7, 14, 0], 2753),
    ("scape25.map", [1, 3, 15, 8, 5, 22, 8, 4, 7, 21, 0], 2753),
]



def parse_battery(output: str):
    for line in output.splitlines():
        if "Coste de Energía" in line:
            try:
                gasto = int(line.split(":")[1])
                return 3000 - gasto
            except ValueError:
                return None
    return None

def color_text(text, color_code):
    return f"\033[{color_code}m{text}\033[0m"

def check_base_messages(output: str):
    rescatador_ok = "El rescatador ha alcanzado un puesto base." in output
    auxiliar_ok = "El auxiliar ha alcanzado un puesto base." in output
    return rescatador_ok, auxiliar_ok

def run_test(map_name, params, expected_battery):
    result = subprocess.run(
        [EXEC, f"{MAP_DIR}/{map_name}"] + list(map(str, params)),
        capture_output=True,
        text=True
    )

    if result.returncode != 0:
        return (False, map_name, params, f"Error: {result.stderr.strip()}")

    level = params[1]
    if level == 0:
        rescatador_ok, auxiliar_ok = check_base_messages(result.stdout)
        if rescatador_ok and auxiliar_ok:
            return (True, map_name, None, "Bases alcanzadas")
        else:
            missing = []
            if not rescatador_ok:
                missing.append("Rescatador")
            if not auxiliar_ok:
                missing.append("Auxiliar")
            return (False, map_name, params, f"No alcanzado: {', '.join(missing)}")

    battery = parse_battery(result.stdout)
    if battery == expected_battery:
        return (True, map_name, None, battery)
    else:
        return (False, map_name, params, battery)

if __name__ == "__main__":
    total = len(tests)
    ok = 0
    failed_tests = []

    current_level = None
    for test in tests:
        level = test[1][1]
        if level != current_level:
            current_level = level
            print(f"\n{color_text(f'Tests nivel {level}:', '34')}")

        success, map_name, params, info = run_test(*test)
        if success:
            print(color_text(f"✅ {map_name}", "32"))
            ok += 1
        else:
            if isinstance(info, str) and "Error" in info:
                print(color_text(f"❌ {map_name} {' '.join(map(str, params))}", "31"))
                print(color_text(f"   {info}", "31"))
            else:
                expected = test[2]
                print(color_text(f"❌ {map_name} {' '.join(map(str, params))}", "31"))
                print(color_text(f"   batería final = {info} (esperado {expected})", "31"))
            failed_tests.append((map_name, params))

    print("=" * 40)
    print(f"Total: {total} | OK: {ok} | Fails: {total - ok}")
    if failed_tests:
        print(color_text("\nTests fallidos:", "31"))
        for map_name, params in failed_tests:
            print(f"{EXEC} {MAP_DIR}/{map_name} {' '.join(map(str, params))}")
