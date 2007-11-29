/* OrixFile Library (c) 2006 Felipe Astroza 
 * Under LGPL 
 */

/* Felipe Astroza: Mejoras en el consumo de memoria, al no hacer una copia de nombre en la memoria,
 * ahora lo obtiene desde la memoria mapeada.
 * Reemplace o_data_edit() por 2 funciones mas utiles o_get_offet() y o_access_to_mem()
 * con o_get_access() obtengo el offset del dato, y luego lo materializo con o_access_to_mem()
 * quien me devuelve un puntero de este.
 * Felipe Astroza: Omití los verificadores de argumentos, por considerarlos demasiado obvios. 
 * Ahora toda la responsabilidad recae en el programador.
 * Felipe Astroza: Mejoras en el consumo de memoria.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>  
#include <fcntl.h>
#include <assert.h>
#define __USE_GNU
#include <unistd.h>
#include <sys/mman.h>

#include <hash.h>
#include <ofile.h>

static void load_file(o_file *);
static int o_get_flags(const char *);

#define OFILE_SIZE(a) (((o_file_header *)(a)->mapped.base)->f_size)
#define OADDR(a, b) ( (caddr_t)(a)->mapped.base + (b) )

/* Obtiene el numero de paginas */
static inline int PAGES(int pagsize, int size)
{
	return (size % pagsize) != 0? (size / pagsize) + 1 : size / pagsize;
}

#define OFILE_PAGES(a) PAGES((a)->pagsize, OFILE_SIZE((a)))

static const char orix_file_header_[] = {'O', 'F', 'L', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

/* Abre un OrixFile. Se devuelve el puntero de una estructura o_file, la cual contiene el file descriptor, cabezera y una tabla hash
   Si el fichero no es nuevo, y contiene datos, se agregan a la tabla hash cada nodo con la informacion de un dato (nombre, offset y size) */
o_file *o_open(const char *file, const char *mode)
{
	int fd;
	o_file *of;
	struct stat st;
	void *addr;
	o_file_header *header;
	int flags;
	int prot = 0;
	int pagsize;
	int zero;

	flags = o_get_flags(mode);

	if(flags == O_RDONLY)
		prot = PROT_READ;
	else if(flags & O_RDWR)
		prot = PROT_READ|PROT_WRITE;

	if(stat(file, &st) == -1) {
		flags |= O_CREAT;
		st.st_mode = 0664;
		printf("%s(): creating new OrixFile\n", __FUNCTION__);
		zero = 1;
	} else {
		if(st.st_size < O_HEADERSIZE) {
			printf("%s(): error: file format is not Orix File\n", __FUNCTION__);
			return NULL;
		}
		zero = 0;
	}
	if( (fd = open(file, flags, st.st_mode)) < 0) {
		perror("open");
		return NULL;
	}
	if(zero) {
		if(write(fd, orix_file_header_, O_HEADERSIZE) == -1) {
			close(fd);
			return NULL;
		}
		st.st_size =  O_HEADERSIZE;
	}

	pagsize = getpagesize();
	addr = mmap(0, PAGES(pagsize, st.st_size) * pagsize, prot, MAP_SHARED, fd, 0);
	if( addr == (void *) -1 ) {
		perror("mmap");
		return NULL;
	}
	header = addr;
	if(zero == 0) {
		if(strncmp(header->fn, "OFL", 3)!=0) {
			close(fd);
			printf("%s(): error: file format is not Orix File\n", __FUNCTION__);
			munmap(header, O_HEADERSIZE);
			return NULL;
		}
	} else
		header->f_size = O_HEADERSIZE;

	if(!(of = calloc(1, sizeof(o_file)))) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	of->mapped.base = addr;
	of->pagsize = pagsize;
	of->mapped.pages = OFILE_PAGES(of);
	of->mapped.prot = prot;
	of->fd = fd;
	of->flags = flags;

	ocore_hash_init(&of->hash, OFILE_HASHSIZE, NULL);
	if(header->num > 0)
		load_file(of);

	return of;
}

static int o_get_flags(const char *m) 
{
	int flags = 0;
	const char *aux;

	if(!m)
		return O_RDONLY; /* default */

	aux = m;
	do {
		if(OF_READ == *aux) {
			flags |= O_RDONLY; /* ... O_RDONLY is 00 */
			continue;
		}
		if(OF_WRITE == *aux) {
			flags |= O_RDWR;
			continue;
		}
		if(OF_TRUNCATE == *aux) {
			flags |= O_TRUNC;
			continue;
		}
	} while(*aux++);

	return flags;
}

/* load_file(): Registra las entradas existentes en el fichero
*/
static void load_file(o_file *of)
{
	char *name;
	off_t offset = O_HEADERSIZE;
	size_t sz = OFILE_SIZE(of);
	o_metadata *md;
	int num = ((o_file_header *)of->mapped.base)->num;

        while ( sz > offset && num > 0 ) {

		md = (o_metadata *)OADDR(of, offset);
		name = (char *)OADDR(of, offset + sizeof(o_metadata));

		if(!ocore_hash_add(&of->hash, name, (void *)offset, 0))
			fprintf(stderr, "%s():\"%s\" already exists\n", __FUNCTION__, name);

							   /* size of data */
		offset += sizeof(o_metadata) + O_NAMESIZE(md) + md->size;
		num--;
	}
}

static void o_update_offset(o_file *of, int since, int adjust)
{
	ocore_hash_position pst;
	ocore_hash_node *node;

	pst.node = NULL;
	pst.idx = 0;

	while( (node = ocore_hash_list(&of->hash, &pst)) ) {
		if(node->value == NULL)
			continue;

		if((off_t)node->value > since)
			*((off_t *)&node->value) -= (off_t)adjust;

		/* De todas formas actualizamos el miembro 'name', por si base cambia de valor */
		node->name = OADDR(of, (off_t)node->value + sizeof(o_metadata));

	}
}

static void o_mremap(o_file *of, int pages)
{
	void *addr;
	size_t new_size = pages * of->pagsize;
	size_t old_size = of->mapped.pages * of->pagsize;

#ifdef linux
	addr = mremap(of->mapped.base, old_size, new_size, MREMAP_MAYMOVE);
	assert(addr != (void *)-1);
#else
#warning "There isn't mremap, using mmap in a ugly implementation"
	/* Este es un reemplazo de mremap temporal, no me gusta */
	munmap(of->mapped.base, old_size);
	addr = mmap(of->mapped.base, new_size, of->mapped.prot, MAP_SHARED, of->fd, 0); /* MAP_FIXED no es necesario */
	assert(addr != (void *)-1);
#endif

	if(addr != of->mapped.base) {
		of->mapped.base = addr;
		o_update_offset(of, 0, 0);
	}

	of->mapped.pages = pages;
}

int o_close(o_file *of)
{
	close(of->fd);
	ocore_hash_free_table(&of->hash);
	munmap(of->mapped.base, OFILE_PAGES(of) * of->pagsize);
	free(of);

	return 1;
}

/* o_write(): Escribe al final del fichero la nueva entrada
 */
static int o_write(o_file *of, const char *name, void *data, o_metadata *md)
{
	o_file_header *header;
	void *dst;
	int pages, old_sz;

	old_sz = OFILE_SIZE(of);

	if(ftruncate(of->fd, old_sz + sizeof(o_metadata) + O_NAMESIZE(md) + md->size) == -1) {
		perror("ftruncate");
		return 0;
	}

	header = of->mapped.base;
	header->num++;
	header->f_size += sizeof(o_metadata) + O_NAMESIZE(md) + md->size;
	pages = OFILE_PAGES(of);
	o_mremap(of, pages);

	dst = OADDR(of, old_sz);
	memcpy(dst, md, sizeof(o_metadata));
	dst = (caddr_t)dst + sizeof(o_metadata); 
	memcpy(dst, name, O_NAMESIZE(md));
	dst = (caddr_t)dst + O_NAMESIZE(md);
	memcpy(dst, data, md->size);

	return old_sz;
}

/* o_write_entry(): Verifica la existencia de una entrada con el mismo nombre, agrega
 * la nueva entrada en la tabla hash y finalmente llama a o_write().
 */
int o_write_entry(o_file *of, const char *name, void *data, size_t size)
{
	ocore_hash_node *node;
	o_metadata md;
	off_t offset;

	if(size == 0)
		return 0;

	if( !(node = ocore_hash_add(&of->hash, name, NULL, 0)) )
		return 0;

	md.namelen = strlen(name);
	md.size = size;

	if( !(offset = o_write(of, name, data, &md)) ) {
		ocore_hash_remove(&of->hash, name);
		return 0;
	}

	node->value = (void *)offset;
	node->name = (char *)OADDR(of, offset + sizeof(o_metadata));

	return size;
}

/* o_read(): Copia 'len' bytes en la direccion 'buf' de la entrada
 */
static int o_read(o_file *of, off_t offset, void *buf, size_t len)
{
	void *src;
	o_metadata *md;

	if(len <= 0)
		return 0;

	md = (o_metadata *)OADDR(of, offset);

	if(md->size > len)
		len = md->size;

	src = OADDR(of, offset + sizeof(o_metadata) + O_NAMESIZE(md));

	memcpy(buf, src, len);

	return len;
}

/* o_read_entry(): Busca la entrada en la tabla hash y llama a o_read().
 */
int o_read_entry(o_file *of, const char *name, void *buf, size_t len) 
{
	off_t offset;

	offset = (off_t)ocore_hash_get_value(&of->hash, name);
	if(!offset)
		return 0;

	return o_read(of, offset, buf, len);
}

static void o_delete(o_file *of, off_t offset)
{
	o_file_header *header;
	o_metadata md;
	int bytes, count, pages;
	int8_t *src,*dst;

	/* 	     E	    M	   M			   	 
	   |------|------|------|------|	|------|------|------|
	   |  1   |  2o<----3o<----4o  | ---->  |  1   |  3   |	 4   |
	   |------|------|------|------|	|------|------|------|
				esa es la idea...
	*/

	memcpy(&md, OADDR(of, offset), sizeof(o_metadata));

	/* Bytes a mover */
	bytes = OFILE_SIZE(of) - (offset + O_SZINFILE(&md));
	count = 0;

	dst = (int8_t *)OADDR(of, offset);
	src = (int8_t *)OADDR(of, offset + O_SZINFILE(&md));

	/* Relocalizacion de los octectos */
	for(count = 0; count < bytes; count++)
		dst[count] = src[count];

	header = of->mapped.base;
	header->num -= 1; /* Numero de elementos disminuye en 1 */
	header->f_size = offset + count; /* "offset + count" es el ultimo byte escrito, por lo tanto es el nuevo tamano */

	ftruncate(of->fd, OFILE_SIZE(of));
	o_update_offset(of, offset, O_SZINFILE(&md));

	pages = OFILE_PAGES(of);
	o_mremap(of, pages);
}

int o_delete_entry(o_file *of, const char *name)
{
	off_t offset;

	if(!(of->flags & O_RDWR))
		return 0;

	offset = (off_t)ocore_hash_get_value(&of->hash, name);
	if(!offset)
		return 0;

	ocore_hash_remove(&of->hash, name);
	o_delete(of, offset);
	return 1;
}

int o_rename_entry(o_file *of, const char *old, const char *new)
{
	void *data, *dst;
	ocore_hash_node *node;
	size_t end, entry_size;
	o_metadata md, *md_p;

	if(!(of->flags & O_RDWR))
		return 0;

	node = ocore_hash_change_key(&of->hash, old, new);
	if(!node)
		return 0;

	md_p = (o_metadata *)OADDR(of, (off_t)node->value);
	md.namelen = strlen(new);
	md.size = md_p->size;

	/* Cuando los nombres son del mismo tamaño,
	 * solo escribo el nuevo sobre el viejo.
	 */
	if(md.namelen == md_p->namelen) {
		/* Escribo el nuevo nombre */
		dst = OADDR(of, (off_t)node->value + sizeof(o_metadata));
		memcpy(dst, new, md.namelen);
		/* Asigno el nombre final */
		node->name = dst;

		return 1;
	}

	/* Salvo el tamaño del fichero (final), para luego saber donde esta la nueva entrada */
	end = OFILE_SIZE(of);

	/* Lo siguiente es escribir la informacion otra vez pero con el nuevo
 	 * nombre y finalmente eliminar la vieja entrada. De esta forma me aseguro de no perder
	 * informacion. La forma errada es rescatar, eliminar, escribir.
	 */

	data = o_access_to_mem(of, (off_t)node->value, NULL);

	if(o_write(of, new, data, &md) < md.size)
		return 0; /* No fue posible cambiar de nombre, se mantiene el viejo */

	entry_size = O_SZINFILE(md_p);

	/* Ultimo argumento 0, para que no libere el nodo que utiliza en la hash table */
	o_delete(of, (off_t)node->value);

	node->value = (void *)(end - entry_size);
	node->name = (char *)( OADDR(of, (off_t)node->value + sizeof(o_metadata)) );

	return 1;
}

off_t o_get_offset(o_file *of, const char *name)
{
	return (off_t)ocore_hash_get_value(&of->hash, name);
}

void *o_access_to_mem(o_file *of, off_t offset, size_t *size)
{
	o_metadata *md;

	if(offset < O_HEADERSIZE || offset > OFILE_SIZE(of))
		return NULL;

	md = (o_metadata *)( OADDR(of, offset) );
	if(size)
		*size = md->size;

	return (void *)( OADDR(of, offset + sizeof(o_metadata) + O_NAMESIZE(md)) );
}
 
int o_touch_entry(o_file *of, const char *name)
{
	off_t offset;
	o_metadata *md;

	offset = (off_t)ocore_hash_get_value(&of->hash, name);
	if(offset == 0)
		return 0;

	md = (o_metadata *)OADDR(of, offset);

	return md->size;
}

void o_clean_up(o_file *of)
{
	ocore_hash_position pst;
	ocore_hash_node *node;

	if(!(of->flags & O_RDWR))
		return;

	if(of) {
		pst.node = NULL;
		pst.idx = 0;

		while( (node = ocore_hash_list(&of->hash, &pst)) ) {
			o_delete(of, (off_t)node->value);
		}

		ocore_hash_destroy_all(&of->hash);
	}

}
