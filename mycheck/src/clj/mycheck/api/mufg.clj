(ns mycheck.api.mufg
  (:require [config.core :refer [env]]
            [clojure.data.json :as json]
            [clj-http.client :as client]
            [schema.core :as s]))

;; Api endpoint
(def apicontext (str (:api-endpoint env) "/v1"))

;; convert string to json
(defn get-body-as-json [response]
  (json/read-str (response :body) :key-fn keyword))

;; account schema
(s/defschema Account {(s/optional-key :id) Long
                      :user_id s/Str
                      :user_name s/Str
                      :phone_number s/Str
                      :account_id s/Str
                      :balance Long})

;; check schema
(s/defschema Check {:id s/Str
                    :account_id s/Str
                    :token s/Str
                    :acc_token s/Str
                    :amount Long
                    :status s/Int
                    :dest (s/maybe s/Str)
                    :dest_acc_id (s/maybe s/Str)})

;; restructure bank user
(defn restructure-user [user]
  (let [accounts (:my_accounts user)
        account (first (filter #(= "1" (:account_type_cd %)) accounts))]
    (-> (apply assoc {}
          (mapcat (fn [k] [k (k user)]) [:user_id :user_name :phone_number]))
        (assoc :account_id (:account_id account)
               :balance (:balance account)))))

;; --- Bank API helper ---
;; OAuth token header
(defn header-token [token]
  {:headers {:Authorization (str "Bearer " token)
             :Content-Type "application/json"}})

;; Checky account
(def checky-account "3454857042")

;; convert check to json string
(defn check-to-json [check]
  (-> (assoc {} :amount (check :amount))
    (assoc :payee {:account_id (check :dest_acc_id)})
    (json/write-str)))

;; Bank API: get user
(defn getuser [token]
  (-> (str apicontext "/users/me")
    (client/get {:headers {:Authorization (str "Bearer " token)}})
    (get-body-as-json)
    (restructure-user)))

;; Bank API: get account
(defn getAccount [token account_id]
  (-> (str apicontext "/accounts/" account_id)
    (client/get (header-token token))
    (get-body-as-json)))

;; Bank API: transfer
(defn transfer [token check]
  (-> (str apicontext "/accounts/" checky-account "/transfers")
    (client/post (assoc (header-token token) :body (check-to-json check) :accept :json))))

;; Bank API: transfer approve
(defn approve [token]
  (-> (str apicontext "/accounts/" checky-account "/transfers?action=approve")
    (client/post (assoc (header-token token) :body "{}" :accept :json))))
