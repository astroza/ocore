/* OrixFile Library (c) 2006 Felipe Astroza
 * Under LGPL
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <hash.h>
#include <ofile.h>

#define BUFSIZE 4096
#define ARGS 32
#define PROMPT_1 0 /* Consola normal */
#define PROMPT_2 1 /* Continuando buffer */

static const char *prompts[] = {"Ofile> ", "> "};
static o_file *of = NULL;
static int status = 1;
static ocore_hash c_hash;
void o_exec(char *);

typedef void (*console_command)(int, char **);

#define N_CMD 8

void cmd_read(int, char **);
void cmd_write(int, char **);
void cmd_delete(int, char **);
void cmd_list(int, char **);
void cmd_rename(int, char **);
void cmd_clean(int, char **);
void cmd_help(int, char **);
void cmd_exit(int, char **);

struct {
	const char *name;
	const char *description;
	console_command func;
} o_commands[] = {
	{"read", "Read entry", cmd_read},
	{"write", "Write entry", cmd_write},
	{"delete", "Delete entry", cmd_delete},
	{"list", "Print every entry name", cmd_list},
	{"rename", "Change name to a entry", cmd_rename},
	{"clean", "Remove all", cmd_clean},
	{"help", "Print help", cmd_help},
	{"exit", "Leave console", cmd_exit}
};

/* from orix/parse.c */
void clean_buf(const char *buf)
{
        char *tmp;

        if(buf) {
                for(tmp = (char *)buf; *tmp; *tmp++)
                        if(*tmp == '\n' || *tmp == '\r')
                                *tmp = 0;
        }
}

/* from orix/parse.c */
char *xstrub(char *string, int len, int s, int e)
{
	if(!string || len < e)
		return NULL;

	if(s != e)
		*(string+e) = 0;

	return (string + s);
}

/* from orix/parse.c 
 * str_to_list():
 * - Pasa una string a una lista, separando por 's'
 * a menos que este entre un "".
 * - Esta limitado por un maximo 'max' de elementos
 */
int str_to_list(char **list, char *buf, char s, int max)
{
        int x, i, j, list_idx;
        int len;
        int reg[] = {0, 0, 0};

        if(!buf)
                return 0;

        len = strlen(buf);

        for(x=0, list_idx=0, i=0; i < len && list_idx < max; i++) {
                if(buf[i] == s && reg[0] == 0) {
                        j=0;
                        while(buf[i+j] != s)
                                j++;
                        x = (i = i+j) + 1;
                }
                if(buf[i] != s && buf[i] != '"')
                        reg[0] = 1;

                if(buf[i] == '"') {
                        if(reg[1] == 0 || reg[2] == 1) {
                                reg[1] = 1;
                                reg[2] = 0;
                        } else
                                reg[2] = 1;
                }

                if(reg[0] == 1 && (reg[1] == 1? reg[2]: 1)) {
                        if(buf[i] == s) {
                                list[list_idx++] = xstrub(buf, len, x+reg[1], i-reg[2]);
                                x = i + 1;
                        } else
                        if(buf[i+1] == 0) {
                                list[list_idx++] = xstrub(buf, len, x+reg[1], i+1-reg[2]);
                                x = i + 1;
                        }

                        if(x > i) {
                                reg[0] = 0;
                                reg[1] = 0;
                                reg[2] = 0;
                        }
                }
        }
        return list_idx;
}

void prepare_console()
{
	int i;

	ocore_hash_init(&c_hash, 32, NULL);
	for(i=0; i < N_CMD; i++) 
		if(ocore_hash_add(&c_hash, (char *)o_commands[i].name, (void *)i, 0) == 0) {
			fprintf(stderr, "can't add command to hash table\n");
			exit(EXIT_FAILURE);
		}
}

int main(int c, char **v)
{
	char buffer[BUFSIZE]="";
	int b_read, len, p, ret, po;
	char *aux;

	printf("OrixFile Console / Felipe Astroza\n");

	if(c < 2)
		return 0;

	of = o_open(v[1], "rw");
	if(!of) {
		fprintf(stderr, "Unable to open file\n");
		return -1;
	}

	b_read = 0;
	p = 0;
	po = PROMPT_1;
	prepare_console();

	/* Por mientras, al continuar un buffer, no se pone un ' ' o '\n' entre medio, que seria lo mejor */
	while(status) {
		fprintf(stdout, "%s", prompts[po]);
		fflush(stdout);

		ret = read(0, buffer + b_read, BUFSIZE - b_read - 1);
		if(ret == 0)
			break;

		clean_buf(buffer + b_read);
		len = strlen(buffer + b_read);
		if(len == 0)
			continue;

		aux = buffer + b_read;

		while(*aux) {
			if(*aux == '"') {
				if(p == 0 && ((buffer + b_read - aux) == 0? 1 : *(aux - 1) == ' ') ) {
					p = 1;
					po = PROMPT_2;
				} else if(p == 1) {
					po = PROMPT_1;
					p = 0;
				}
			}
			*aux++;
		}

		b_read += len;
		if(b_read >= BUFSIZE - 1 && p == 1) {
			fprintf(stderr, "string too long\n");
			b_read = 0;
			p = 0;
			continue;
		}

		/* Listo para ser analizado */
		if(p == 0) {
			o_exec(buffer);
			b_read = 0;
		} else
			continue;

	}

	o_close(of);
	return 0;
}

void o_exec(char *buf)
{
	char *argv[ARGS];
	int argc, i;
	ocore_hash_node *node;

	memset(argv, 0, sizeof(char *) * ARGS);
	argc = str_to_list(argv, buf, ' ', ARGS);

	node = ocore_hash_get_node(&c_hash, argv[0]);
	if(node) {
		i = (int)node->value;
		o_commands[i].func(argc, argv);
	} else
		fprintf(stderr, "%s: command not found\n", argv[0]);
}

void cmd_read(int argc, char **argv)
{
	off_t offset;
	char *data;
	size_t size = 0;

	if(argc < 2 || argc > 2) {
		fprintf(stdout, "%s [name]\n", argv[0]);
		return;
	}

	offset = o_get_offset(of, argv[1]);
	if(offset == 0) {
		fprintf(stderr, "%s: entry \"%s\" not found\n", argv[0], argv[1]);
		return;
	}

	data = o_access_to_mem(of, offset, &size);
	write(1, data, size);
	write(1, "\n", 1);
}

void cmd_write(int argc, char **argv)
{
	if(argc < 3 || argc > 3) {
		fprintf(stdout, "%s [name] [string]\n", argv[0]);
		return;
	}

	if(o_write_entry(of, argv[1], argv[2], strlen(argv[2])) == 0) {
		fprintf(stderr, "%s: can't add entry\n", argv[0]);
		return;
	}

	fprintf(stdout, "DONE\n");
}

void cmd_delete(int argc, char **argv)
{
	if(argc < 2 || argc > 2) {
		fprintf(stdout, "%s [name]\n", argv[0]);
		return;
	}

	if(o_delete_entry(of, argv[1]) == 0) {
		fprintf(stderr, "%s: entry \"%s\" not found\n", argv[0], argv[1]);
		return;
	}

	fprintf(stdout, "DONE\n");
}

void cmd_list(int argc, char **argv)
{
	ocore_hash_node *node;
	ocore_hash_position pst;

	pst.node = NULL;
	pst.idx = 0;

	while( (node = ocore_hash_list(&of->hash, &pst)) )
		printf("%s\n", node->name);

}

void cmd_rename(int argc, char **argv)
{
	if(argc < 3 || argc > 3) {
		fprintf(stdout, "%s [name] [newname]\n", argv[0]);
		return;
	}

	if(o_rename_entry(of, argv[1], argv[2]))
		fprintf(stdout, "OK\n");
	else
		fprintf(stdout, "Can't rename entry\n");
}

void cmd_clean(int argc, char **argv)
{
	o_clean_up(of);
}

void cmd_help(int argc, char **argv)
{
	int i;

	for(i=0; i < N_CMD; i++)
		printf("%s\t:%s\n", o_commands[i].name, o_commands[i].description);

}

void cmd_exit(int argc, char **argv)
{
	status = 0;
}
