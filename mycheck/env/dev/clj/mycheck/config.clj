(ns mycheck.config
  (:require [selmer.parser :as parser]
            [clojure.tools.logging :as log]
            [mycheck.dev-middleware :refer [wrap-dev]]))

(def defaults
  {:init
   (fn []
     (parser/cache-off!)
     (log/info "\n-=[mycheck started successfully using the development profile]=-"))
   :middleware wrap-dev})
