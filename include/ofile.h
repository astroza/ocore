/* Felipe Astroza 2006
 * Ocore ofile.h
 * Under GPL
 */
#ifndef __O_FILE_
#define __O_FILE_

#define OF_READ		'r'	
#define OF_WRITE	'w'
#define OF_TRUNCATE	't'

typedef struct {
	char fn[3]; /* nombre del formato */
	size_t f_size;
	int num;
} o_file_header;

#define OFILE_HASHSIZE 32

#define O_HEADERSIZE	sizeof(o_file_header)

/* + 1 por el ultimo byte agregado cuyo valor es 0 */ 
#define O_NAMESIZE(a) ((a)->namelen + 1)

/* con entry_inf, consigue la cantidad de bytes que ocupa la entrada
 * en el fichero
 */
#define O_SZINFILE(a) (sizeof(o_metadata) + O_NAMESIZE(a) + (a)->size)

/* Tamaño de los datos */
#define O_ENTRYSIZE(a) ((a)->size)

/* informacion de la informacion.. Contiene el tamaño de dato 
 * y la longitud del nombre 
 */
typedef struct {
	size_t size;
	size_t namelen;
} o_metadata;

typedef struct {
	int fd;
	int flags;
	int pagsize;

	struct 
	{
		void *base;
		int pages;
		int prot;
	} mapped;

	ocore_hash hash;

} o_file;

o_file *o_open(const char *, const char *);
int o_close(o_file *);
int o_write_entry(o_file *, const char *, void *, size_t);
int o_read_entry(o_file *, const char *, void *, size_t);
int o_delete_entry(o_file *, const char *);
int o_rename_entry(o_file *, const char *, const char *);
off_t o_get_offset(o_file *, const char *);
void *o_access_to_mem(o_file *, off_t, size_t *);
int o_touch_entry(o_file *, const char *);
void o_clean_up(o_file *);

#endif
