#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <assert.h>
#include <pwd.h>
#include "parse_mail.h"

#define DEFAULT_HOST "localhost"
#define DEFAULT_SERV "12301"
#define MAX_PACKET_SIZE 8192
#define BUFFER_SIZE 4096

#define OPTIONS "df:g:hi:Nnu:v"

static const char *progname;

static void
usage(void)
{
    fprintf(stderr,
            "usage: %s [OPTION]... [host [port]]\n"
            "    -i file\tread the message from file rather than stdin\n"
            "    -f folder\tinclude the folder name in the notification\n"
            "    -g group\tpost message to a tickertape group too\n"
            "    -d\t\tproduce a hexdump of the notification\n"
            "    -n\t\tdon't actually send the notification\n"
            "    -N\t\tdon't copy stdin to stdout\n"
            "    -u user\tsend notification for user\n"
            "    -h\t\tprint this brief help message\n"
            "    -v\t\tprint version information and exit\n",
            progname);
}

/* Print out the bytes of a stream */
void
fhexdump(char *buffer, size_t length, FILE *out)
{
    char *point;
    int i, ch;

    /* Go through the buffer in 16-byte chunks */
    for (point = buffer - ((intptr_t)buffer & 0xf);
         point < buffer + length;
         point += 16) {
        fprintf(out, "%04tx:", point - buffer);

        /* Print the 16 bytes as hex */
        for (i = 0; i < 16; i++) {
            if (i % 8 == 0) {
                fputc(' ', out);
            }

            if (buffer <= point + i && point + i < buffer + length) {
                fprintf(out, "%02x ", (unsigned char)*(point + i));
            } else {
                fprintf(out, "   ");
            }
        }

        /* Print the 16 bytes as ASCII */
        fputc('>', out);
        for (i = 0; i < 16; i++) {
            if (buffer <= point + i && point + i < buffer + length) {
                ch = (unsigned char)*(point + i);
                if (ch < 32 || ch > 126) {
                    fputc('.', out);
                } else {
                    fputc(ch, out);
                }
            } else {
                fputc(' ', out);
            }
        }

        fprintf(out, "<\n");
    }
}

static void
xwrite(int fd, const char *buffer, size_t len)
{
    const char *point, *end;
    ssize_t res;

    point = buffer;
    end = point + len;
    while (point < end) {
        if ((res = write(fd, point, end - point)) < 0) {
            exit(1);
        }

        assert(res != 0);
        point += res;
    }
}

int main(int argc, char *argv[])
{
    struct lexer lexer;
    struct addrinfo *addrinfo, *addr, hints;
    struct passwd *pwent;
    char packet[MAX_PACKET_SIZE];
    char buffer[BUFFER_SIZE];
    char *host = DEFAULT_HOST;
    char *serv = DEFAULT_SERV;
    char *path = NULL;
    char *folder = NULL;
    char *group = NULL;
    char *user = NULL;
    char *point;
    int err, choice;
    int fd = STDIN_FILENO;
    int sock = -1;
    ssize_t len;
    int do_hexdump = 0;
    int do_send = 1;
    int do_pipe = 1;

    /* Determine the basename of the executable */
    point = strrchr(argv[0], '/');
    progname = point ? point + 1 : argv[0];

    /* Parse the command-line options */
    while ((choice = getopt(argc, argv, OPTIONS)) != -1) {
        switch (choice) {
        case 'd':
            do_hexdump = 1;
            break;

        case 'f':
            folder = optarg;
            break;

        case 'g':
            group = optarg;
            break;

        case 'h':
            usage();
            exit(0);

        case 'i':
            if (path != NULL) {
                fprintf(stderr, "%s: error: only one input file may be named\n", progname);
                exit(1);
            }

            path = optarg;
            if ((fd = open(path, O_RDONLY)) < 0) {
                fprintf(stderr, "%s: error: unable to open file %s: %s\n",
                        progname, path, strerror(errno));
                exit(1);
            }
            break;

        case 'n':
            do_send = 0;
            break;

        case 'N':
            do_pipe = 0;
            break;

        case 'u':
            user = optarg;
            break;

        case 'v':
             printf("%s (" PACKAGE ") version " VERSION "\n", progname);
            exit(0);

        case '?':
            fprintf(stderr, "%s: error: unknown option -%c\n", progname, optopt);
            exit(1);

        default:
            abort();
        }
    }

    /* Set the filename if reading from stdin */
    path = path ? path : "<stdin>";

    /* See if a host name was given */
    if (optind < argc) {
        host = argv[optind++];
    }

    /* See if a port was named */
    if (optind < argc) {
        serv = argv[optind++];
    }

    /* Complain about additional arguments */
    if (optind < argc) {
        fprintf(stderr, "%s: error: superfluous argument: %s\n",
                progname, argv[optind]);
        usage();
        exit(1);
    }

    /* If no user was specified then guess one. */
    if (user == NULL) {
        user = getenv("USER");
    }

    if (user == NULL) {
        user = getenv("LOGNAME");
    }

    if (user == NULL) {
        pwent = getpwuid(getuid());
        if (pwent) {
            user = pwent->pw_name;
        }
    }

    if (user == NULL) {
        user = "unknown";
    }

    if (!do_send) {
        addr = NULL;
    } else {
        /* Look up the address */
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_DGRAM;
        if ((err = getaddrinfo(host, serv, &hints, &addrinfo)) != 0) {
            fprintf(stderr, "%s: error: unable to resolve host %s:%s\n",
                    progname, host, serv);
            exit(1);
        }

        /* Create a socket */
        for (addr = addrinfo; addr; addr = addr->ai_next) {
            sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
            if (sock < 0) {
                continue;
            }
            break;
        }
        if (addr == NULL) {
            fprintf(stderr, "%s: error: unable to create socket: %s\n",
                    progname, strerror(errno));
            exit(1);
        }
    }

    /* Initialize the lexer */
    lexer_init(&lexer, packet, sizeof(packet));
    lexer_append_unotify_header(&lexer, user, folder, group);

    /* Digest the message while copying it to stdout */
    do {
        if ((len = read(fd, buffer, sizeof(buffer))) < 0) {
            fprintf(stderr, "%s: unable to read from file %s: %s\n",
                    progname, path, strerror(errno));
            exit(1);
        }

        if (lex(&lexer, buffer, len) < 0) {
            fprintf(stderr, "%s: error parsing file %s: %d\n",
                    progname, path, errno);
            break;
        }

        /* Copy the input to stdout */
        if (do_pipe) {
            xwrite(STDOUT_FILENO, buffer, len);
        }
    } while (len != 0);

    /* Add the footer */
    lexer_append_unotify_footer(&lexer, -1);

    /* Close the input file */
    if (close(fd) < 0) {
        fprintf(stderr, "%s: error: unable to close file %s: %s\n",
                progname, path, strerror(errno));
    }

    if (do_send) {
        assert(sock != -1);

        /* Send the notification */
        if (sendto(sock, packet, lexer.point - packet, 0,
                   addr->ai_addr, addr->ai_addrlen) < 0) {
            fprintf(stderr, "%s: error: unable to send packet: %s\n",
                    progname, strerror(errno));
            exit(1);
        }

        if (close(sock) < 0) {
            fprintf(stderr, "%s: error: unable to close socket: %s\n",
                    progname, strerror(errno));
            exit(1);
        }
        sock = -1;
        freeaddrinfo(addrinfo);
    }

    if (do_hexdump) {
        /* Produce a hexdump of the notification */
        fhexdump(packet, lexer_size(&lexer), stdout);
    }

    exit(0);
}
