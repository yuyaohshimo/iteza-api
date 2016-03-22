(ns mycheck.db.core
  (:require
    [yesql.core :refer [defqueries]]
    [config.core :refer [env]]))

(def conn
  {:classname   "org.h2.Driver"
   :connection-uri (:database-url env)
   :make-pool?     true
   :naming         {:keys   clojure.string/lower-case
                    :fields clojure.string/upper-case}})

(defqueries "sql/account_queries.sql" {:connection conn})
(defqueries "sql/check_queries.sql" {:connection conn})
(defqueries "sql/token_queries.sql" {:connection conn})
