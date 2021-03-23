// ==============================================================================
/**
 * mmu.c
 */
// ==============================================================================



// ==============================================================================
// INCLUDES

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "mmu.h"
#include "vmsim.h"
// ==============================================================================



// ==============================================================================
// MACROS AND GLOBALS

/** 
 * The (real) address of the upper page table.  Initialized by a call
 * to `mmu_init()`.
 */
static vmsim_addr_t upper_pt_addr = 0;
// ==============================================================================



// ==============================================================================
void
mmu_init (vmsim_addr_t new_upper_pt_addr) {

  upper_pt_addr = new_upper_pt_addr;
  
}
// ==============================================================================



// ==============================================================================
vmsim_addr_t
mmu_translate (vmsim_addr_t sim_addr) {

  //get upper pointer address 
  vmsim_addr_t upr_ptr_addr = upper_pt_addr + ((sim_addr >> 22) & 0x3ff)*(sizeof(pt_entry_t));
  //upper table entry
  pt_entry_t upr_ptr = 0;
  vmsim_read_real(&upr_ptr, upr_ptr_addr, sizeof(upr_ptr));

  //There is no LPT to which the address’s UPT entry leads, so call vmsim_map_fault, and try again
  if(upr_ptr == 0){
    vmsim_map_fault(sim_addr);
    return mmu_translate(sim_addr);
  }
  //pointer to the lowr table
  vmsim_addr_t lwr_pt_addr = (upr_ptr & ~0xfff);
  //getting the lower table entry
  vmsim_addr_t lwr_ptr_addr = lwr_pt_addr + (sim_addr >> 12)*sizeof(pt_entry_t);
pt_entry_t lwr_ptr = 0;
vmsim_read_real(&lwr_ptr, lwr_ptr_addr, sizeof(lwr_ptr));


//There is no real page to whichthe address’s LPT entry leads, so call vmsim_fault, and retry
if(lwr_ptr == 0){
    vmsim_map_fault(sim_addr);
    return mmu_translate(sim_addr);
  }

// get the real address, by combining, simulated address and offset
vmsim_addr_t real_addr = (lwr_ptr & ~0xfff) | (sim_addr & 0xfff);
return real_addr;
}
// ==============================================================================
