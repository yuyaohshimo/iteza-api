(ns user
  (:require [mycheck.handler :refer [app init destroy]]
            [luminus.http-server :as http]
            [config.core :refer [env]]))

;; this file used for repl. see project.clj

(defn start []
  (http/start {:handler app
               :init    init
               :port    (:port env)}))

(defn stop []
  (http/stop destroy))

(defn restart []
  (stop)
  (start))
