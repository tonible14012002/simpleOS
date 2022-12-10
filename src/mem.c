
#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

static struct
{
	uint32_t proc; // ID of process currently uses this page
	int index;	   // Index of the page in the list of pages allocated
				   // to the process.
	int next;	   // The next page in the list. -1 if it is the last
				   // page.
} _mem_stat[NUM_PAGES];

static pthread_mutex_t mem_lock;

void init_mem(void)
{
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* get offset of the virtual address */
static addr_t get_offset(addr_t addr)
{
	return addr & ~((~0U) << OFFSET_LEN);
}

/* get the first layer index */
static addr_t get_first_lv(addr_t addr)
{
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* get the second layer index */
static addr_t get_second_lv(addr_t addr)
{
	return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
}

/* Search for page table table from the a segment table */
static struct trans_table_t *get_trans_table(
	addr_t index, // Segment level index
	struct page_table_t *page_table)
{ // first level table

	/*
	 * TODO: Given the Segment index [index], you must go through each
	 * row of the segment table [page_table] and check if the v_index
	 * field of the row is equal to the index
	 *
	 * */
	int i;
	for (i = 0; i < page_table->size; i++)
	{
		// Enter your code here
		if (page_table->table[i].v_index == index)
		{
			return page_table->table[i].next_lv;
		}
	}
	return NULL;
}

/* Translate virtual address to physical address. If [virtual_addr] is valid,
 * return 1 and write its physical counterpart to [physical_addr].
 * Otherwise, return 0 */
	static int translate(
		addr_t virtual_addr,   // Given virtual address
		addr_t *physical_addr, // Physical addresqs to be returned
		struct pcb_t *proc)
	{ // Process uses given virtual address

		/* Offset of the virtual address */
		addr_t offset = get_offset(virtual_addr);
		/* The first layer index */
		addr_t first_lv = get_first_lv(virtual_addr);
		/* The second layer index */
		addr_t second_lv = get_second_lv(virtual_addr);

		/* Search in the first level */
		struct trans_table_t * trans_table = NULL;
		trans_table = get_trans_table(first_lv, proc->page_table);
		if (trans_table == NULL)
		{
			return 0;
		}
		int i;
		for (i = 0; i < trans_table->size; i++)
		{
			if (trans_table->table[i].v_index == second_lv)
			{
				/* TODO: Concatenate the offset of the virtual addess
				* to [p_index] field of trans_table->table[i] to
				* produce the correct physical address and save it to
				* [*physical_addr]  */
				*physical_addr = (trans_table->table[i].p_index << OFFSET_LEN) | offset;
				return 1;
			}
		}
		return 0;
	}
/**
 * We allocate memory to a process by updating the `_mem_stat` array, and updating the page table of
 * the process
 * 
 * @param size the size of the memory region to be allocated
 * @param proc the process that is requesting memory
 * 
 * @return The address of the first byte in the allocated memory region.
 */
addr_t alloc_mem(uint32_t size, struct pcb_t *proc)
{
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;
	/* TODO: Allocate [size] byte in the memory for the
	 * process [proc] and save the address of the first
	 * byte in the allocated memory region to [ret_mem].
	 * */

	uint32_t num_pages = (size % PAGE_SIZE == 0) ? size / PAGE_SIZE : size / PAGE_SIZE + 1; // Number of pages we will use
	int mem_avail = 0;																		// We could allocate new memory region or not?

	/* First we must check if the amount of free memory in
	 * virtual address space and physical address space is
	 * large enough to represent the amount of required
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */
	uint32_t num_free_pages = 0;
	for (int i = 0; i < NUM_PAGES; i++)
	{
		if (_mem_stat[i].proc == 0)
		{
			num_free_pages++;
		}
	}
	if (num_free_pages >= num_pages && proc->bp + num_pages * PAGE_SIZE < RAM_SIZE)
		mem_avail = 1;

	if (mem_avail)
	{
		/* We could allocate new memory region to the process */
		ret_mem = proc->bp;
		proc->bp += num_pages * PAGE_SIZE;
		/* Update status of physical pages which will be allocated
		 * to [proc] in _mem_stat. Tasks to do:
		 * 	- Update [proc], [index], and [next] field
		 * 	- Add entries to segment table page tables of [proc]
		 * 	  to ensure accesses to allocated memory slot is
		 * 	  valid. */
		uint32_t num_allocated_pages = 0;
		for (int p_i = 0, v_i = 0, prev_p_i = 0; p_i < NUM_PAGES; p_i++)
		{
			if (_mem_stat[p_i].proc == 0)
			{
				/* This is the code that updates the `_mem_stat` array. */
				_mem_stat[p_i].proc = proc->pid;
				_mem_stat[p_i].index = v_i;
				if (v_i != 0)
					_mem_stat[prev_p_i].next = p_i;
				/* Getting the physical address of the page, the page index and the translation index. */

				addr_t physical_addr = p_i << OFFSET_LEN;
				addr_t page_idx = get_first_lv(ret_mem + v_i * PAGE_SIZE);
				addr_t trans_idx = get_second_lv(ret_mem + v_i * PAGE_SIZE);

				struct trans_table_t *table = get_trans_table(page_idx, proc->page_table);
				/* This is the code that adds a new entry to the page table. */
				if (table)
				{
					table->table[table->size].v_index = trans_idx;
					table->table[table->size].p_index = physical_addr >> OFFSET_LEN;
					table->size++;
				}
				else
				{
				/* This is the code that adds a new entry to the page table. */
					struct page_table_t *t = proc->page_table;
					int n = t->size;
					t->size++;
					t->table[n].next_lv = (struct trans_table_t *)malloc(sizeof(struct trans_table_t));
					t->table[n].next_lv->size++;
					t->table[n].v_index = page_idx;
					t->table[n].next_lv->table[0].v_index = trans_idx;
					t->table[n].next_lv->table[0].p_index = physical_addr >> OFFSET_LEN;
				}

				/* This is the code that updates the `_mem_stat` array. */
				prev_p_i = p_i;
				v_i++;
				num_allocated_pages++;
				/* This is to ensure that the last page in the allocated memory region has a `next` value of `-1`. */
				if (num_allocated_pages == num_pages)
				{
					_mem_stat[prev_p_i].next = -1;
					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&mem_lock);
	// printf("-----Allocated-----------------------------------------\n");
	// dump();
	return ret_mem;
}

int free_mem(addr_t address, struct pcb_t *proc)
{
	/*TODO: Release memory region allocated by [proc]. The first byte of
	 * this region is indicated by [address]. Task to do:
	 * 	- Set flag [proc] of physical page use by the memory block
	 * 	  back to zero to indicate that it is free.
	 * 	- Remove unused entries in segment table and page tables of
	 * 	  the process [proc].
	 * 	- Remember to use lock to protect the memory from other
	 * 	  processes.  */
	pthread_mutex_lock(&mem_lock);
	addr_t phys_addr;
	if (translate(address, &phys_addr, proc))
	{
		int v_i = 0;
		int index = phys_addr >> OFFSET_LEN;
		struct page_table_t *t = proc->page_table;
		while (index != -1)
		{
			_mem_stat[index].proc = 0;
			addr_t page_idx = get_first_lv(address + v_i * PAGE_SIZE);
			addr_t trans_idx = get_second_lv(address + v_i * PAGE_SIZE);
			for (int n = 0; n < t->size; n++)
			{
				if (t->table[n].v_index == page_idx)
				{
					for (int m = 0; m < t->table[n].next_lv->size; m++)
					{
						if (t->table[n].next_lv->table[m].v_index == trans_idx)
						{
							int k = 0;
							for (k = m; k < t->table[n].next_lv->size - 1; k++)
							{
								t->table[n].next_lv->table[k].v_index = t->table[n].next_lv->table[k + 1].v_index;
								t->table[n].next_lv->table[k].p_index = t->table[n].next_lv->table[k + 1].p_index;
							}
							t->table[n].next_lv->table[k].v_index = 0;
							t->table[n].next_lv->table[k].p_index = 0;
							t->table[n].next_lv->size--;
							break;
						}
					}
					if (t->table[n].next_lv->size == 0)
					{
						free(t->table[n].next_lv);
						int m = 0;
						for (m = n; m < t->size - 1; m++)
						{
							t->table[m].v_index = t->table[m + 1].v_index;
							t->table[m].next_lv = t->table[m + 1].next_lv;
						}
						t->table[m].v_index = 0;
						t->table[m].next_lv = NULL;
						t->size--;
					}
					break;
				}
			}
			v_i++;
			index = _mem_stat[index].next;
		}
	}
	pthread_mutex_unlock(&mem_lock);
	// printf("-----Freed---------------------------------------------\n");
	// dump();
	return 0;
}

int read_mem(addr_t address, struct pcb_t *proc, BYTE *data)
{
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc))
	{
		*data = _ram[physical_addr];
		return 0;
	}
	else
	{
		return 1;
	}
}

int write_mem(addr_t address, struct pcb_t *proc, BYTE data)
{
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc))
	{
		_ram[physical_addr] = data;
		return 0;
	}
	else
	{
		return 1;
	}
}

void dump(void)
{
	int i;
	for (i = 0; i < NUM_PAGES; i++)
	{
		if (_mem_stat[i].proc != 0)
		{
			printf("%03d: ", i);
			printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
				   i << OFFSET_LEN,
				   ((i + 1) << OFFSET_LEN) - 1,
				   _mem_stat[i].proc,
				   _mem_stat[i].index,
				   _mem_stat[i].next);
			int j;
			for (j = i << OFFSET_LEN;
				 j < ((i + 1) << OFFSET_LEN) - 1;
				 j++)
			{

				if (_ram[j] != 0)
				{
					printf("\t%05x: %02x\n", j, _ram[j]);
				}
			}
		}
	}
}

