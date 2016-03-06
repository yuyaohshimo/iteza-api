(ns mycheck.config
  (:require [clojure.tools.logging :as log]))

(def defaults
  {:init
   (fn []
     (log/info "\n-=[mycheck started successfully]=-"))
   :middleware identity})
