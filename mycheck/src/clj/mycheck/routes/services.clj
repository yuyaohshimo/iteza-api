(ns mycheck.routes.services
  (:require [ring.util.http-response :refer :all]
            [compojure.api.sweet :refer :all]
            [schema.core :as s]
            [clojure.tools.logging :as log]
            [clj-http.client :as client]
            [clojure.data.json :as json]
            [config.core :refer [env]]
            [mycheck.db.core :as db])
  (:use     [slingshot.slingshot :only [try+ throw+]]))

;; Api endpoint
(def apicontext (str (:api-endpoint env) "/v1"))

;; convert string to json
(defn get-body-as-json [response]
  (json/read-str (response :body)))

;; account schema
(s/defschema Account {:id (s/maybe Long)
                      :user_id s/Str
                      :user_name s/Str
                      :phone_number s/Str
                      :account_id s/Str
                      :balance Long})

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

;; utility

;; sec 6 digits
(defn sec-str []
  (let [sec (str (System/currentTimeMillis))
        len (count sec)]
    (subs sec (- len 6) len)))

;; check id generator
(defn id-gen [account_id]
  (str (subs account_id 3) (sec-str)))

;; common wrapper for api call
(defn wrap-api [call]
  (try+
    (call)
    (catch [:status 401] {:keys [request-time headers body :as e]}
      (log/warn (str e))
      (unauthorized body))))

;; --- account functions ---

;; check accounts exists and if there'nt, create.
(defn check-and-create-account! [token]
  (let [user (getuser token)]
    (if-let [account (first (db/get-account-by-user {:user_id (user :user_id)}))]
      account
      ;; else
      (do
        (db/create-accounts! user)
        user))))

;; --- check functions ---
(defn create-check! [id token auth amt]
  (db/create-check! {:id id
                     :token token
                     :acc_token auth
                     :amount amt}))

;; service definitions
(defapi service-routes
  {:swagger {:ui "/swagger-ui"
             :spec "/swagger.json"
             :data {:info {:version "1.0.0"
                           :title "Checky API"
                           :description "じぶん小切手 API"}}}}
  (context "/api" []
    (POST "/auth" []
      :tags ["debug"]
      :return       {:access_token s/Str}
      :form-params   [access_token :- s/Str]
      :summary      "OAuthのコールバック用。開発用"
      (do
        (log/info (str "/auth: token=" access_token))
        (ok {:access_token access_token})))

    (GET "/checky" []
      :tags ["user" "checky"]
      :return       (s/maybe Account)
      :header-params [auth_token :- s/Str]
      :summary      "ユーザー情報と小切手口座情報を返す"
      (do
        (log/info (str auth_token))
        (wrap-api
          (fn [] (ok (check-and-create-account! auth_token))))))

    ;; 小切手発行API
    (POST "/checky/:id/issue" []
      :tags ["checky"]
      :return       {:account (s/maybe Account)
                     :checkid s/Str}
      :path-params  [id :- s/Str]
      :header-params [auth_token :- s/Str]
      :form-params  [amount :- Long]
      :summary      "小切手の発行"
      (let [user (getuser auth_token)
            accid {:account_id id}
            token "1111"
            account (first (db/get-account-by-accid accid))]
        (if account
          (do
            (create-check! id token auth_token amount)
            ;; TODO: 口座更新
            ;; 更新された口座を返す
            (let [u_account (first (db/get-account-by-accid accid))]
              (ok {:account u_account :checkid (str id token)})))
          (not-found {:errors {:msg "checky口座が存在しません" :id id}}))))

    ;; 小切手受け取りAPI
    (POST "/check/receive" []
      :tags ["receive"]
      :return {:msg s/Str}
      :body [id {:id s/Str :receiver s/Str}]
      :summary "小切手の受け取り"
      (ok {:msg "受け取りました"}))

    ;; 口座入力API
    (POST "/check/checkin" []
      :tags ["receive"]
      :return {:msg s/Str}
      :body [id {:receiver s/Str :account_no s/Str}]
      :summary "送金先口座登録"
      (ok {:msg "設定しました"}))

    ;; 小切手決済
    (POST "/check/settle" []
      :tags ["receive"]
      :return {:msg s/Str}
      :body [id {:receiver s/Str}]
      :summary "小切手の決済"
      (ok {:msg "決済しました"}))))
