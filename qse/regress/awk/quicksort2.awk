BEGIN   { 
         recurse1 = "../../cmd/awk/qseawk -f quicksort2.awk #" rand()
         recurse2 = "../../cmd/awk/qseawk -f quicksort2.awk #" rand()
        }
NR == 1 { 
	pivot=$0; 
	next 
}
NR > 1  { if($0 < pivot) { print | recurse1 }
          if($0 > pivot) { print | recurse2 }
        }
END     { 
          close(recurse1)
          if(NR > 0) print pivot
          close(recurse2)
        }

