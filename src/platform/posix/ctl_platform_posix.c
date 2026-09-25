/*
 * Implementação de plataforma para sistemas POSIX: Linux e
 * Android/Termux compartilham este mesmo arquivo, já que ambos expõem
 * terminais compatíveis com UTF-8 nativo e sequências de escape ANSI
 * para cor, além de pthreads para o mutex. Não há necessidade de
 * separar Android em pasta própria: a única diferença de Termux para
 * Linux "puro" é a libc (Bionic vs glibc/musl), que é transparente
 * neste nível de código.
 */
#if !defined(_POSIX_C_SOURCE)
/* Necessário para expor fileno() em modo estritamente C11/C17 (ver
 * mesmo comentário em ctl_log.c). Precisa vir ANTES de qualquer
 * #include. */
#define _POSIX_C_SOURCE 200809L
#endif

#include "../ctl_platform.h"

#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

struct ctl_mutex {
    pthread_mutex_t handle;
};

/* Converte uma ctl_color_t para o código de escape ANSI correspondente.
 * "\x1b[0m" (reset) é usado tanto para CTL_COLOR_DEFAULT quanto após
 * qualquer texto colorido, para não "vazar" cor para o resto do
 * terminal. */
static const char *ctl_ansi_code(ctl_color_t color) {
    switch (color) {
        case CTL_COLOR_BLACK:          return "\x1b[30m";
        case CTL_COLOR_RED:            return "\x1b[31m";
        case CTL_COLOR_GREEN:          return "\x1b[32m";
        case CTL_COLOR_YELLOW:         return "\x1b[33m";
        case CTL_COLOR_BLUE:           return "\x1b[34m";
        case CTL_COLOR_MAGENTA:        return "\x1b[35m";
        case CTL_COLOR_CYAN:           return "\x1b[36m";
        case CTL_COLOR_WHITE:          return "\x1b[37m";
        case CTL_COLOR_BRIGHT_BLACK:   return "\x1b[90m";
        case CTL_COLOR_BRIGHT_RED:     return "\x1b[91m";
        case CTL_COLOR_BRIGHT_GREEN:   return "\x1b[92m";
        case CTL_COLOR_BRIGHT_YELLOW:  return "\x1b[93m";
        case CTL_COLOR_BRIGHT_BLUE:    return "\x1b[94m";
        case CTL_COLOR_BRIGHT_MAGENTA: return "\x1b[95m";
        case CTL_COLOR_BRIGHT_CYAN:    return "\x1b[96m";
        case CTL_COLOR_BRIGHT_WHITE:   return "\x1b[97m";
        default:                       return "\x1b[0m";
    }
}

void ctl_platform_init(void) {
    /* Nada a fazer: terminais Linux/Android/Termux já trabalham
     * nativamente em UTF-8 e entendem sequências ANSI sem configuração
     * adicional. */
}

bool ctl_platform_isatty(FILE *stream) {
    return isatty(fileno(stream)) != 0;
}

void ctl_platform_write(FILE *stream, ctl_color_t color, bool use_color, const char *utf8_text) {
    if (use_color && color != CTL_COLOR_DEFAULT) {
        fputs(ctl_ansi_code(color), stream);
        fputs(utf8_text, stream);
        fputs("\x1b[0m", stream);
    } else {
        fputs(utf8_text, stream);
    }
    /* fflush garante que a linha apareça imediatamente mesmo quando
     * stdout está em modo "fully buffered" (comum quando a saída é
     * redirecionada para arquivo/pipe em vez de um terminal). */
    fflush(stream);
}

ctl_mutex_t *ctl_platform_mutex_create(void) {
    ctl_mutex_t *m = (ctl_mutex_t *)malloc(sizeof(*m));
    if (m) {
        pthread_mutex_init(&m->handle, NULL);
    }
    return m;
}

void ctl_platform_mutex_lock(ctl_mutex_t *m) {
    if (m) {
        pthread_mutex_lock(&m->handle);
    }
}

void ctl_platform_mutex_unlock(ctl_mutex_t *m) {
    if (m) {
        pthread_mutex_unlock(&m->handle);
    }
}

void ctl_platform_mutex_destroy(ctl_mutex_t *m) {
    if (m) {
        pthread_mutex_destroy(&m->handle);
        free(m);
    }
}
