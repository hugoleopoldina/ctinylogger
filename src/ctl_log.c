/*
 * Núcleo comum a todas as funções de log. Não contém nada específico de
 * plataforma: monta a linha de texto final (timestamp + prefixo +
 * [arquivo:linha (função)] quando aplicável + mensagem formatada) e
 * delega a escrita de fato para a camada de plataforma
 * (src/platform/ctl_platform.h).
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
/* Necessário para expor localtime_r() em modo estritamente C11/C17: sem
 * isso, glibc/Bionic só declaram a API ISO C, que não inclui as
 * extensões POSIX thread-safe (localtime_r). Precisa vir ANTES de
 * qualquer #include. */
#define _POSIX_C_SOURCE 200809L
#endif

#include "ctinylogger/ctinylogger.h"
#include "ctl_internal.h"
#include "platform/ctl_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

/* Extrai só o nome do arquivo de um caminho completo (ex:
 * "/home/user/proj/src/main.c" -> "main.c"), sem alocar memória nem
 * modificar a string original. Procura tanto '/' (Linux/Android) quanto
 * '\' (Windows), já que __FILE__ pode vir em qualquer um dos dois
 * formatos dependendo do compilador/SO usado para compilar. */
static const char *ctl_basename(const char *path) {
    const char *slash = strrchr(path, '/');
    const char *bslash = strrchr(path, '\\');
    const char *last = slash;
    if (bslash && (!last || bslash > last)) {
        last = bslash;
    }
    return last ? last + 1 : path;
}

/* Formata a hora atual em 'out' (tamanho 'out_size') usando o formato
 * strftime configurado. Usa localtime_r (POSIX) / localtime_s (Windows)
 * em vez de localtime() puro, pois este último usa um buffer estático
 * interno e não é thread-safe. */
static void ctl_format_timestamp(char *out, size_t out_size, const char *fmt) {
    time_t now = time(NULL);
    struct tm tm_buf;
#if defined(_WIN32)
    localtime_s(&tm_buf, &now);
#else
    localtime_r(&now, &tm_buf);
#endif
    strftime(out, out_size, fmt, &tm_buf);
}

/* Formata fmt/args (estilo printf) em um buffer alocado dinamicamente,
 * capaz de comportar mensagens de qualquer tamanho (sem truncar).
 * Técnica: uma primeira passagem com vsnprintf(NULL, 0, ...) apenas
 * mede quantos bytes seriam necessários; aloca exatamente esse tanto; a
 * segunda passagem escreve de fato. Precisamos de va_copy porque uma
 * va_list só pode ser percorrida com segurança uma vez.
 * O chamador é responsável por liberar o buffer retornado com free().
 * Retorna NULL em caso de erro de formatação ou falta de memória. */
static char *ctl_vformat(const char *fmt, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
    if (needed < 0) {
        return NULL;
    }

    char *buf = (char *)malloc((size_t)needed + 1);
    if (!buf) {
        return NULL;
    }

    vsnprintf(buf, (size_t)needed + 1, fmt, args);
    return buf;
}

/* Monta a linha final de log e a escreve via ctl_platform_write().
 * file/func só são relevantes (não-NULL) para CTL_LEVEL_DEBUG. */
static void ctl_log_dispatch(ctl_level_t level, const char *file, int line,
                              const char *func, const char *fmt, va_list args) {
    ctl_config_t *cfg = ctl_config_get();
    if (!cfg->initialized) {
        ctl_config_init();
    }

    if (level < cfg->min_level) {
        return; /* nível filtrado pela configuração (ctl_set_min_level) */
    }

    ctl_lock();

    char *message = ctl_vformat(fmt, args);
    if (!message) {
        ctl_unlock();
        return;
    }

    char timestamp[32] = {0};
    if (cfg->timestamp_enabled) {
        ctl_format_timestamp(timestamp, sizeof(timestamp), cfg->timestamp_fmt);
    }

    /* "arquivo:linha (função) " -- só preenchido para log_debug. Note o
     * espaço final, que separa esse bloco da mensagem em si. */
    char location[256] = {0};
    if (level == CTL_LEVEL_DEBUG && file && func) {
        snprintf(location, sizeof(location), "%s:%d (%s) ", ctl_basename(file), line, func);
    }

    /* Buffer fixo para o caso comum (linha curta): evita malloc() na
     * maioria das chamadas de log. Se a linha não couber aqui,
     * recalculamos com um buffer dinâmico do tamanho exato abaixo. */
    char line_buf[1024];
    int written;
    if (cfg->timestamp_enabled) {
        written = snprintf(line_buf, sizeof(line_buf), "[%s] %s %s%s\n",
                            timestamp, cfg->levels[level].prefix, location, message);
    } else {
        written = snprintf(line_buf, sizeof(line_buf), "%s %s%s\n",
                            cfg->levels[level].prefix, location, message);
    }

    if (written < 0) {
        free(message);
        ctl_unlock();
        return; /* erro de formatação, nada a fazer */
    }

    FILE *stream = (level >= CTL_LEVEL_WARNING && cfg->errors_to_stderr) ? stderr : stdout;

    bool use_color;
    switch (cfg->color_mode) {
        case CTL_COLOR_MODE_ON:  use_color = true; break;
        case CTL_COLOR_MODE_OFF: use_color = false; break;
        default:                 use_color = ctl_platform_isatty(stream); break;
    }

    if ((size_t)written >= sizeof(line_buf)) {
        /* Linha maior que o buffer fixo (mensagem muito longa): aloca
         * dinamicamente o tamanho exato reportado por snprintf. */
        char *dyn = (char *)malloc((size_t)written + 1);
        if (dyn) {
            if (cfg->timestamp_enabled) {
                snprintf(dyn, (size_t)written + 1, "[%s] %s %s%s\n",
                         timestamp, cfg->levels[level].prefix, location, message);
            } else {
                snprintf(dyn, (size_t)written + 1, "%s %s%s\n",
                         cfg->levels[level].prefix, location, message);
            }
            ctl_platform_write(stream, cfg->levels[level].color, use_color, dyn);
            free(dyn);
        }
    } else {
        ctl_platform_write(stream, cfg->levels[level].color, use_color, line_buf);
    }

    free(message);
    ctl_unlock();
}

void ctl_log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ctl_log_dispatch(CTL_LEVEL_INFO, NULL, 0, NULL, fmt, args);
    va_end(args);
}

void ctl_log_warning(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ctl_log_dispatch(CTL_LEVEL_WARNING, NULL, 0, NULL, fmt, args);
    va_end(args);
}

void ctl_log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ctl_log_dispatch(CTL_LEVEL_ERROR, NULL, 0, NULL, fmt, args);
    va_end(args);
}

void ctl_log_debug_impl(const char *file, int line, const char *func, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ctl_log_dispatch(CTL_LEVEL_DEBUG, file, line, func, fmt, args);
    va_end(args);
}
