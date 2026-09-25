/*
 * Exemplo de uso da ctinylogger. Compilado automaticamente pelo
 * CMakeLists.txt (alvo "ctinylogger_example") a menos que
 * CTINYLOGGER_BUILD_EXAMPLE=OFF seja passado ao configurar o projeto.
 */
#include <ctinylogger/ctinylogger.h>

int main(void) {
    /* Opcional: só é necessário se você quiser configurar algo antes do
     * primeiro log. Caso contrário, a lib se auto-inicializa com
     * valores padrão na primeira chamada de log_*(). */
    ctl_config_init();

    /* Suporte a UTF-8: acentos, emojis e caracteres largos funcionam
     * normalmente em qualquer uma das três plataformas suportadas. */
    log_info("Aplicação iniciada. Versão %s ✅", "1.0.0");

    log_debug("Valor calculado: %d (esperado: %d)", 42, 42);

    log_warning("Configuração '%s' ausente, usando padrão de %d segundos", "timeout", 30);

    log_error("Falha ao abrir arquivo '%s' (código de erro %d)", "config.json", 2);

    /* Personalizando cor e prefixo de um nível específico. */
    ctl_set_color(CTL_LEVEL_INFO, CTL_COLOR_BRIGHT_MAGENTA);
    ctl_set_prefix(CTL_LEVEL_INFO, "[APP]");
    log_info("Esta linha usa cor e prefixo customizados.");

    /* Desligando cores manualmente (por padrão elas já desligam sozinhas
     * quando a saída não é um terminal, ex: `./exemplo > log.txt`). */
    ctl_set_color_mode(CTL_COLOR_MODE_OFF);
    log_info("Esta linha é impressa sem cores, mesmo em um terminal.");
    ctl_set_color_mode(CTL_COLOR_MODE_AUTO);

    /* Filtrando por nível mínimo: log_debug deixa de aparecer. */
    ctl_set_min_level(CTL_LEVEL_INFO);
    log_debug("Esta linha de debug NÃO deve aparecer.");
    log_info("Já esta linha de info continua aparecendo normalmente.");

    return 0;
}
