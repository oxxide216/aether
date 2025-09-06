(def str 'hello world')

(defun print-args [i]
  (if (ls i (get-args-count))
    (print (get-arg i))
    (print-args (add i 1))))

(print-args 0)
(print str)
