#include "types.h"
#include "stat.h"
#include "user.h"

#define REF_LEN 20

// The exact reference string from your exam prompt
int ref_string[REF_LEN] = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1};

// Helper function to visually print the frames array
void print_frames(int *frames, int num_frames) {
  int i;
  for (i = 0; i < num_frames; i++) {
    if (frames[i] == -1) 
      printf(1, "[ ] ");
    else 
      printf(1, "[%d] ", frames[i]);
  }
}

// Implements FIFO Page Replacement
int run_fifo(int num_frames, int verbose) {
  int frames[10];
  int faults = 0;
  int oldest = 0;
  int i, j, page, hit;

  for(i = 0; i < num_frames; i++) {
    frames[i] = -1; // Initialize empty frames
  }
  
  if(verbose) printf(1, "\n--- FIFO with %d frames ---\n", num_frames);

  for(i = 0; i < REF_LEN; i++) {
    page = ref_string[i];
    hit = 0;
    
    for(j = 0; j < num_frames; j++) {
      if(frames[j] == page) {
        hit = 1;
        break;
      }
    }

    if(!hit) {
      frames[oldest] = page;
      oldest = (oldest + 1) % num_frames;
      faults++;
    }

    if(verbose) {
      printf(1, "Ref %d: ", page);
      print_frames(frames, num_frames);
      printf(1, " -> %s\n", hit ? "Hit" : "Fault");
    }
  }
  return faults;
}

// Implements Least Recently Used (LRU) Page Replacement
int run_lru(int num_frames, int verbose) {
  int frames[10];
  int last_used[10];
  int faults = 0;
  int i, j, page, hit, hit_idx, lru_idx, min_time;

  for(i = 0; i < num_frames; i++) {
    frames[i] = -1;
    last_used[i] = -1;
  }
  
  if(verbose) printf(1, "\n--- LRU with %d frames ---\n", num_frames);

  for(i = 0; i < REF_LEN; i++) {
    page = ref_string[i];
    hit = 0;
    hit_idx = -1;
    
    for(j = 0; j < num_frames; j++) {
      if(frames[j] == page) {
        hit = 1;
        hit_idx = j;
        break;
      }
    }

    if(hit) {
      last_used[hit_idx] = i; // Update the time it was used
    } else {
      lru_idx = 0;
      min_time = last_used[0];
      for(j = 1; j < num_frames; j++) {
        if(last_used[j] < min_time) {
          min_time = last_used[j];
          lru_idx = j;
        }
      }
      frames[lru_idx] = page;
      last_used[lru_idx] = i; // Record the time it was inserted
      faults++;
    }

    if(verbose) {
      printf(1, "Ref %d: ", page);
      print_frames(frames, num_frames);
      printf(1, " -> %s\n", hit ? "Hit" : "Fault");
    }
  }
  return faults;
}

int main(void) {
  int fifo_3, lru_3, f, fifo_f, lru_f, fifo_hit, lru_hit;

  // Parts (a) & (b): Show step-by-step logic and prove 3 frames yields 15 and 12 faults
  fifo_3 = run_fifo(3, 1);
  printf(1, "\nTotal FIFO Faults (3 frames): %d (Expected: 15)\n", fifo_3);

  lru_3 = run_lru(3, 1);
  printf(1, "\nTotal LRU Faults (3 frames): %d (Expected: 12)\n", lru_3);

  // Part (c): Print summary table for 3, 4, and 5 frames
  printf(1, "\n--- Summary Table (Total Refs = %d) ---\n", REF_LEN);
  printf(1, "Frames | FIFO Faults | FIFO Hit%% | LRU Faults | LRU Hit%%\n");
  printf(1, "--------------------------------------------------------\n");
  
  for(f = 3; f <= 5; f++) {
    fifo_f = run_fifo(f, 0);
    lru_f = run_lru(f, 0);
    
    fifo_hit = ((REF_LEN - fifo_f) * 100) / REF_LEN;
    lru_hit = ((REF_LEN - lru_f) * 100) / REF_LEN;

    printf(1, "   %d   |      %d     |    %d%%    |     %d     |   %d%%\n", 
           f, fifo_f, fifo_hit, lru_f, lru_hit);
  }
  
  exit();
}