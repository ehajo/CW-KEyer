#include "fifo.h"

void fifo_init (fifo_t *f, uint8_t *buffer, const uint8_t size)
{
	f->buffer_start=buffer;
	f->count = 0;
	f->pread = f->pwrite = buffer;
	f->read2end = f->write2end = f->size = size;
}

void fifo_reset(fifo_t* f)
{
	uint8_t sreg = SREG;
	cli();
	f->count = 0;
	f->pread = f->pwrite = f->buffer_start;
	f->read2end = f->write2end = f->size;
	SREG = sreg;
}

uint8_t fifo_put (fifo_t *f, const uint8_t data)
{
	return _inline_fifo_put (f, data);
}

uint8_t fifo_get_wait (fifo_t *f)
{
	while (!f->count);
	
	return _inline_fifo_get (f);	
}

int fifo_get_nowait (fifo_t *f)
{
	if (!f->count)		return -1;
		
	return (int) _inline_fifo_get (f);	
}

uint8_t fifo_get_item_count(fifo_t *f)
{
	return f->count; 
}

uint8_t fifo_get_free_count(fifo_t *f)
{
	return ((f->size)-(f->count));
}


int fifo_peek_nowait(fifo_t *f)
{
	if(f->count<=0)
	{
		return -1;
	}
	uint8_t *pread = f->pread;
	uint8_t data = *(pread);
	return data;
}