(def str 'hello world')

(defun print-args [argc argv i]
  (print (add argv i))
  (if (less i argc)
    (print-args argc argv (add i 1))))

(defun main [argc argv]
  (print-args argc argv 0)
  (print str))
