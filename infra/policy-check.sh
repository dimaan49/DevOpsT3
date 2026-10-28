#!/usr/bin/env bash
# Проверка требований к инфраструктурному коду (см. POLICY.md).
# Запускается локально и в CI; любой найденный нарушитель => код возврата 1.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

fail=0
check() { # check "описание" команда-поиска-нарушений
    local title="$1"; shift
    local out
    out="$("$@" 2>/dev/null || true)"
    if [ -n "$out" ]; then
        echo "FAIL: ${title}"
        echo "$out" | sed 's/^/    /'
        fail=1
    else
        echo "ok:   ${title}"
    fi
}

# 1. Нет shell-команд: только модули (command допустим с changed_when, это проверяет ansible-lint).
check "no ansible shell module" \
    bash -c "find . -name '*.yml' -print0 | xargs -0 grep -nE 'ansible\.builtin\.shell|^\s*(- )?shell:\s*[^/ ]|^\s*shell:\s*\$'"

# 2. Повторяемость: нельзя state: latest.
check "no 'state: latest'" \
    bash -c "find . -name '*.yml' -print0 | xargs -0 grep -nE 'state:\s*latest'"

# 3. Минимальные права: нет world-writable режимов.
check "no world-writable modes" \
    bash -c "find . -name '*.yml' -print0 | xargs -0 grep -nE 'mode:\s*\"?0?[0-7][0-7]([2367])\"?\s*$'"

# 4. Нет лишних портов: правила файрвола не открывают порт всему миру.
check "no firewall rule open to the world" \
    bash -c "grep -rnE \"(from|from_ip):\s*['\\\"]?(any|0\.0\.0\.0/0)\" inventories group_vars roles --include='*.yml'"

# 5. Нет лишних портов: публикация портов контейнеров только на конкретный адрес.
check "container ports are bound to a specific IP" \
    bash -c "grep -rnE '^\s*-\s*\"[0-9]+:[0-9]+\"' roles --include='*.j2'; grep -rnE '\"0\.0\.0\.0:[0-9]+' roles --include='*.j2'"

# 6. Секреты: пароли/ключи в group_vars только как ссылка на vault_*.
check "no plaintext secrets in vars" \
    bash -c "grep -rnEi '^[a-z_]*(password|secret|token|private_key)[a-z_]*:\s*[\"'\\'']?[^{\"'\\'' ]' inventories group_vars --include='*.yml' | grep -v 'vault.yml'"

# 7. Секреты: vault.yml, если есть, обязан быть зашифрован.
for f in inventories/*/group_vars/all/vault.yml; do
    [ -e "$f" ] || continue
    if head -1 "$f" | grep -q '^\$ANSIBLE_VAULT;'; then
        echo "ok:   $f is encrypted"
    else
        echo "FAIL: $f is NOT encrypted"
        fail=1
    fi
done

exit "$fail"
