(ns mycheck.routes.services
  (:require [ring.util.http-response :refer :all]
            [compojure.api.sweet :refer :all]
            [schema.core :as s]
            [clojure.tools.logging :as log]
            [clj-http.client :as client]
            [clojure.data.json :as json]
            [config.core :refer [env]])
  (:use     [slingshot.slingshot :only [try+ throw+]]))

;; Api endpoint
(def apicontext (str (:api-endpoint env) "/v1"))

;; convert string to json
(defn get-body-as-json [response]
  (json/read-str (response :body)))

;; restructure bank user
(defn restructure-user [user]
  (let [accounts (get user "my_accounts")
        account (first (filter #(= "1" (get % "account_type_cd")) accounts))]
    (-> (apply assoc {}
          (mapcat (fn [k] [(keyword k) (get user k)]) ["user_id" "user_name" "phone_number"]))
        (assoc :account_id (get account "account_id")
               :balance (get account "balance")))))

;; Bank API: get user
(defn getuser [token]
  (-> (str apicontext "/users/me")
    (client/get {:headers {:Authorization (str "Bearer " token)}})
    (get-body-as-json)
    (restructure-user)))


;; account schema
(s/defschema Account {:user_id s/Str
                      :user_name s/Str
                      :phone_number s/Str
                      :account_id s/Str
                      :balance Long})

;; common wrapper for api call
(defn wrap-api [call]
  (try+
    (call)
    (catch [:status 401] {:keys [request-time headers body :as e]}
      (log/warn (str e))
      (unauthorized body))))

;; service definitions
(defapi service-routes
  {:swagger {:ui "/swagger-ui"
             :spec "/swagger.json"
             :data {:info {:version "1.0.0"
                           :title "Checky API"
                           :description "じぶん小切手 API"}}}}
  (context "/api" []
    (POST "/auth" []
      :tags ["auth" "dev"]
      :return       {:access_token s/Str}
      :form-params   [access_token :- s/Str]
      :summary      "OAuthのコールバック用。開発用"
      (do
        (log/info (str "/auth: token=" access_token))
        (ok {:access_token access_token})))

    (GET "/checky" []
      :tags ["user"]
      :return       (s/maybe Account)
      :header-params [auth_token :- s/Str]
      :summary      "ユーザー情報と小切手口座情報を返す"
      (do
        (log/info (str auth_token))
        (wrap-api
          (fn [] (ok (getuser auth_token))))))

    (POST "/checky/:id/issue" []
      :tags ["checky"]
      :return       {:account (s/maybe Account)}
      :path-params  [id :- s/Str]
      :header-params [auth_token :- s/Str]
      :form-params  [amount :- Long]
      :summary      "小切手の発行"
      (wrap-api
        (fn [] (ok {:account (getuser auth_token)}))))))
