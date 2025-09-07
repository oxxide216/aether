(fun print-list [list]
  (if (is-empty list)
    (println)
   else
    (print (head list) ' ')
    (print-list (tail list))))

(def list [0 1 2])

(print-list list)
(println list)
