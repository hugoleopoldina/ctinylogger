/*
 * Cabeçalho interno, NÃO faz parte da API pública (não é instalado).
 * Compartilhado entre ctl_log.c e ctl_config.c.
 */
#ifndef CTL_INTERNAL_H
#define CTL_INTERNAL_H

#include "ctinylogger/ctinylogger.h"
#include <stdbool.h>

/* Configuração de um único nível de log: cor e prefixo textual. */
typedef struct {
    ctl_color_t color;
    char prefix[32];
} ctl_level_cfg_t;

/* Estrutura de configuração global da lib. Existe uma única instância
 * (estática, em ctl_config.c), acessada via ctl_config_get(). */
typedef struct {
    ctl_level_cfg_t levels[CTL_LEVEL_COUNT];
    ctl_color_mode_t color_mode;
    bool timestamp_enabled;
    char timestamp_fmt[32];
    ctl_level_t min_level;
    bool thread_safe;
    bool errors_to_stderr;
    bool initialized;
} ctl_config_t;

/* Ponteiro para a configuração global (definida em ctl_config.c). */
ctl_config_t *ctl_config_get(void);

/* Trava/destrava o mutex interno da lib. Viram no-op automaticamente
 * quando thread_safe == false (ver implementação em ctl_config.c). */
void ctl_lock(void);
void ctl_unlock(void);

#endif /* CTL_INTERNAL_H */
