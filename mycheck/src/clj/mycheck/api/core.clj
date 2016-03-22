(ns mycheck.api.core
  (:require [clojure.data.json :as json]))

;; convert string to json
(defn get-body-as-json [response]
  (json/read-str (response :body) :key-fn keyword))

;; convert body string to json for httpkit
(defn get-json [future]
  (let [response @future]
    (assoc response :body (get-body-as-json response))))
