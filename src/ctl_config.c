/*
 * Gerenciamento da configuração global da lib (cores, prefixos,
 * timestamp, nível mínimo, thread-safety, etc.) e do mutex interno
 * usado para proteger tanto a configuração quanto a escrita dos logs.
 */
#include "ctinylogger/ctinylogger.h"
#include "ctl_internal.h"
#include "platform/ctl_platform.h"

#include <string.h>
#include <stdlib.h>

/* Única instância de configuração da lib (estado global "de propósito":
 * um logger é, por natureza, um serviço único e compartilhado dentro do
 * processo). Zero-inicializada estaticamente -> initialized == false
 * até a primeira chamada de ctl_config_init(). */
static ctl_config_t g_config;

/* Mutex interno, criado sob demanda na primeira inicialização. */
static ctl_mutex_t *g_mutex = NULL;

ctl_config_t *ctl_config_get(void) {
    return &g_config;
}

void ctl_lock(void) {
    if (g_config.thread_safe && g_mutex) {
        ctl_platform_mutex_lock(g_mutex);
    }
}

void ctl_unlock(void) {
    if (g_config.thread_safe && g_mutex) {
        ctl_platform_mutex_unlock(g_mutex);
    }
}

void ctl_config_init(void) {
    if (g_config.initialized) {
        return;
    }

    /* Inicializa a camada de plataforma primeiro (ex: no Windows,
     * configura o codepage de saída para UTF-8). */
    ctl_platform_init();

    memset(&g_config, 0, sizeof(g_config));

    strncpy(g_config.levels[CTL_LEVEL_DEBUG].prefix,   "[DEBUG]",   sizeof(g_config.levels[0].prefix) - 1);
    strncpy(g_config.levels[CTL_LEVEL_INFO].prefix,    "[INFO]",    sizeof(g_config.levels[0].prefix) - 1);
    strncpy(g_config.levels[CTL_LEVEL_WARNING].prefix, "[WARNING]", sizeof(g_config.levels[0].prefix) - 1);
    strncpy(g_config.levels[CTL_LEVEL_ERROR].prefix,   "[ERROR]",   sizeof(g_config.levels[0].prefix) - 1);

    g_config.levels[CTL_LEVEL_DEBUG].color   = CTL_COLOR_CYAN;
    g_config.levels[CTL_LEVEL_INFO].color    = CTL_COLOR_GREEN;
    g_config.levels[CTL_LEVEL_WARNING].color = CTL_COLOR_YELLOW;
    g_config.levels[CTL_LEVEL_ERROR].color   = CTL_COLOR_BRIGHT_RED;

    g_config.color_mode = CTL_COLOR_MODE_AUTO;
    g_config.timestamp_enabled = true;
    strncpy(g_config.timestamp_fmt, "%H:%M:%S", sizeof(g_config.timestamp_fmt) - 1);
    g_config.min_level = CTL_LEVEL_DEBUG;
    g_config.thread_safe = true;
    g_config.errors_to_stderr = true;

    if (!g_mutex) {
        g_mutex = ctl_platform_mutex_create();
    }

    /* Marcado por último: só a partir daqui thread_safe/g_mutex valem
     * a pena ser considerados por ctl_lock()/ctl_unlock(). */
    g_config.initialized = true;
}

void ctl_set_color(ctl_level_t level, ctl_color_t color) {
    if (level < 0 || level >= CTL_LEVEL_COUNT) {
        return;
    }
    if (!g_config.initialized) {
        ctl_config_init();
    }
    ctl_lock();
    g_config.levels[level].color = color;
    ctl_unlock();
}

void ctl_set_prefix(ctl_level_t level, const char *prefix) {
    if (level < 0 || level >= CTL_LEVEL_COUNT || !prefix) {
        return;
    }
    if (!g_config.initialized) {
        ctl_config_init();
    }
    ctl_lock();
    strncpy(g_config.levels[level].prefix, prefix, sizeof(g_config.levels[0].prefix) - 1);
    g_config.levels[level].prefix[sizeof(g_config.levels[0].prefix) - 1] = '\0';
    ctl_unlock();
}

void ctl_set_color_mode(ctl_color_mode_t mode) {
    if (!g_config.initialized) {
        ctl_config_init();
    }
    ctl_lock();
    g_config.color_mode = mode;
    ctl_unlock();
}

void ctl_set_timestamp_enabled(bool enabled) {
    if (!g_config.initialized) {
        ctl_config_init();
    }
    ctl_lock();
    g_config.timestamp_enabled = enabled;
    ctl_unlock();
}

void ctl_set_timestamp_format(const char *strftime_fmt) {
    if (!strftime_fmt) {
        return;
    }
    if (!g_config.initialized) {
        ctl_config_init();
    }
    ctl_lock();
    strncpy(g_config.timestamp_fmt, strftime_fmt, sizeof(g_config.timestamp_fmt) - 1);
    g_config.timestamp_fmt[sizeof(g_config.timestamp_fmt) - 1] = '\0';
    ctl_unlock();
}

void ctl_set_min_level(ctl_level_t level) {
    if (level < 0 || level > CTL_LEVEL_ERROR) {
        return;
    }
    if (!g_config.initialized) {
        ctl_config_init();
    }
    ctl_lock();
    g_config.min_level = level;
    ctl_unlock();
}

void ctl_set_thread_safe(bool enabled) {
    if (!g_config.initialized) {
        ctl_config_init();
    }
    /* Cria o mutex sob demanda caso ainda não exista (ex: usuário
     * chamou ctl_set_thread_safe(true) após ter desligado antes). Não
     * precisamos de lock aqui: esta função só altera um bool e,
     * possivelmente, cria um mutex novo -- não há dado protegido em
     * jogo além do próprio flag. */
    if (enabled && !g_mutex) {
        g_mutex = ctl_platform_mutex_create();
    }
    g_config.thread_safe = enabled;
}

void ctl_set_errors_to_stderr(bool enabled) {
    if (!g_config.initialized) {
        ctl_config_init();
    }
    ctl_lock();
    g_config.errors_to_stderr = enabled;
    ctl_unlock();
}
