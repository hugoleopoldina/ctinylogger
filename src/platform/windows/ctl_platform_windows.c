/*
 * Implementação de plataforma para Windows (7, 10, 11).
 *
 * Diferente do POSIX, NÃO usamos sequências de escape ANSI para cor:
 * embora o Windows 10+ as suporte via Virtual Terminal Processing, o
 * console clássico do Windows 7 (e o "cmd.exe" legado em versões mais
 * novas, dependendo de configuração) não entende essas sequências e
 * acaba imprimindo lixo (as sequências aparecem literalmente na tela).
 *
 * Em vez disso, usamos a API nativa do Console do Windows:
 *   - SetConsoleTextAttribute para cor (suportado desde o Windows NT)
 *   - WriteConsoleW para o texto em si, que trabalha nativamente com
 *     UTF-16 e nunca sofre com problemas de codepage/mojibake que
 *     afetam printf/WriteFile em consoles antigos com texto UTF-8.
 *
 * Quando a saída é redirecionada para arquivo/pipe (não é mais um
 * "console" de verdade), caímos de volta para escrita direta em UTF-8
 * via fputs, já que WriteConsoleW só funciona em handles de console.
 */
#include "../ctl_platform.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <string.h>

struct ctl_mutex {
    CRITICAL_SECTION handle;
};

/* Converte ctl_color_t para os bits de atributo de foreground do
 * console do Windows (combinação de FOREGROUND_RED/GREEN/BLUE e
 * FOREGROUND_INTENSITY para as variantes "brilhantes"). */
static WORD ctl_win_color_attr(ctl_color_t color) {
    switch (color) {
        case CTL_COLOR_BLACK:          return 0;
        case CTL_COLOR_RED:            return FOREGROUND_RED;
        case CTL_COLOR_GREEN:          return FOREGROUND_GREEN;
        case CTL_COLOR_YELLOW:         return FOREGROUND_RED | FOREGROUND_GREEN;
        case CTL_COLOR_BLUE:           return FOREGROUND_BLUE;
        case CTL_COLOR_MAGENTA:        return FOREGROUND_RED | FOREGROUND_BLUE;
        case CTL_COLOR_CYAN:           return FOREGROUND_GREEN | FOREGROUND_BLUE;
        case CTL_COLOR_WHITE:          return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        case CTL_COLOR_BRIGHT_BLACK:   return FOREGROUND_INTENSITY;
        case CTL_COLOR_BRIGHT_RED:     return FOREGROUND_RED | FOREGROUND_INTENSITY;
        case CTL_COLOR_BRIGHT_GREEN:   return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case CTL_COLOR_BRIGHT_YELLOW:  return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case CTL_COLOR_BRIGHT_BLUE:    return FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case CTL_COLOR_BRIGHT_MAGENTA: return FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case CTL_COLOR_BRIGHT_CYAN:    return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case CTL_COLOR_BRIGHT_WHITE:   return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        default:                       return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
}

/* Retorna o HANDLE de console associado a um FILE* (só stdout/stderr
 * são suportados, que é tudo que a lib usa). */
static HANDLE ctl_handle_for(FILE *stream) {
    return (stream == stderr) ? GetStdHandle(STD_ERROR_HANDLE)
                               : GetStdHandle(STD_OUTPUT_HANDLE);
}

void ctl_platform_init(void) {
    /* Define o codepage de saída como UTF-8. Isso só afeta o caminho de
     * fallback (saída redirecionada, ver ctl_platform_write abaixo);
     * a escrita via WriteConsoleW usa UTF-16 diretamente e é imune a
     * qualquer configuração de codepage, por isso funciona
     * corretamente mesmo no console clássico do Windows 7. */
    SetConsoleOutputCP(CP_UTF8);
}

bool ctl_platform_isatty(FILE *stream) {
    HANDLE h = ctl_handle_for(stream);
    DWORD mode;
    return h != INVALID_HANDLE_VALUE && h != NULL && GetConsoleMode(h, &mode) != 0;
}

void ctl_platform_write(FILE *stream, ctl_color_t color, bool use_color, const char *utf8_text) {
    HANDLE h = ctl_handle_for(stream);
    DWORD mode;
    bool is_console = h != INVALID_HANDLE_VALUE && h != NULL && GetConsoleMode(h, &mode) != 0;

    if (!is_console) {
        /* Saída redirecionada para arquivo/pipe: não há console para
         * usar WriteConsoleW, então escrevemos o UTF-8 puro. */
        fputs(utf8_text, stream);
        fflush(stream);
        return;
    }

    /* Passo 1: descobre quantos wchar_t são necessários. Passo 2:
     * converte de fato. -1 em cbMultiByte faz a função tratar a string
     * de entrada como terminada em NUL e incluir o terminador na
     * contagem/saída. */
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_text, -1, NULL, 0);
    if (wlen <= 0) {
        return;
    }
    wchar_t *wbuf = (wchar_t *)malloc((size_t)wlen * sizeof(wchar_t));
    if (!wbuf) {
        return;
    }
    MultiByteToWideChar(CP_UTF8, 0, utf8_text, -1, wbuf, wlen);

    CONSOLE_SCREEN_BUFFER_INFO info;
    bool have_info = GetConsoleScreenBufferInfo(h, &info) != 0;
    WORD original_attrs = have_info ? info.wAttributes : 0;

    if (use_color && color != CTL_COLOR_DEFAULT && have_info) {
        /* Preserva os bits de cor de fundo (background) já em uso,
         * trocando apenas o foreground (texto). Os 4 bits baixos de
         * wAttributes são o foreground; os 4 seguintes, o background. */
        WORD bg = info.wAttributes & 0xFFF0;
        SetConsoleTextAttribute(h, ctl_win_color_attr(color) | bg);
    }

    DWORD written;
    /* wlen - 1 exclui o terminador NUL, que WriteConsoleW não espera
     * que seja escrito como caractere visível. */
    WriteConsoleW(h, wbuf, (DWORD)(wlen - 1), &written, NULL);

    if (use_color && color != CTL_COLOR_DEFAULT && have_info) {
        SetConsoleTextAttribute(h, original_attrs);
    }

    free(wbuf);
}

ctl_mutex_t *ctl_platform_mutex_create(void) {
    ctl_mutex_t *m = (ctl_mutex_t *)malloc(sizeof(*m));
    if (m) {
        InitializeCriticalSection(&m->handle);
    }
    return m;
}

void ctl_platform_mutex_lock(ctl_mutex_t *m) {
    if (m) {
        EnterCriticalSection(&m->handle);
    }
}

void ctl_platform_mutex_unlock(ctl_mutex_t *m) {
    if (m) {
        LeaveCriticalSection(&m->handle);
    }
}

void ctl_platform_mutex_destroy(ctl_mutex_t *m) {
    if (m) {
        DeleteCriticalSection(&m->handle);
        free(m);
    }
}
