(ns mycheck.routes.services
  (:require [ring.util.http-response :refer :all]
            [compojure.api.sweet :refer :all]
            [schema.core :as s]
            [clojure.tools.logging :as log]
            [config.core :refer [env]]
            [mycheck.db.core :as db])
  (:use     [slingshot.slingshot :only [try+ throw+]]
            [mycheck.api.twillio]
            [mycheck.api.mufg]))

;; utility

;; sec 6 digits
(defn sec-str []
  (let [sec (str (System/currentTimeMillis))
        len (count sec)]
    (subs sec (- len 6) len)))

;; check id generator
(defn id-gen [account_id sec]
  (str (subs account_id 3) sec))

;; common wrapper for api call
(defn wrap-api [call]
  (try+
    (call)
    (catch [:status 401] {:keys [request-time headers body :as e]}
      (log/warn (str e))
      (unauthorized body))
    (catch [:status 400] {:keys [request-time headers body :as e]}
      (log/warn (str e))
      (bad-request body))))

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

;; create a new check
(defn create-check! [account_id token auth amt]
  (db/create-check! {:id (id-gen account_id token)
                     :account_id account_id
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
            token (sec-str)
            account (first (db/get-account-by-accid accid))
            new-balance (if account (- (account :balance) amount) -1)]
        (cond
          (nil? account) (not-found {:errors {:msg "checky口座が存在しません" :id id}})
          (> 0 new-balance) (bad-request {:errors {:msg "口座残高が不足しています" :id id :amount amount}})
          :else
            (do
              (create-check! id token auth_token amount)
              ;; なんでid で更新してaccount_id で引くのか...
              (db/update-balance! {:id (account :id) :balance new-balance})
              (let [u_account (first (db/get-account-by-accid accid))]
                (ok {:account u_account :checkid (id-gen id token)}))))))

    ;; 小切手受け取りAPI
    (POST "/check/receive" []
      :tags ["receive"]
      :return {:msg s/Str}
      :body [rcv {:id s/Str :receiver s/Str}]
      :summary "小切手の受け取り"
      (let [id (rcv :id)
            ;; TODO: なんでマップから取り出してマップに詰めるんだ...
            check (first (db/get-check-by-key {:id id}))
            status (if check (check :status) nil)]
        (cond
          (nil? check) (not-found {:msg "小切手がありません" :id id})
          (not= 0 status) (bad-request {:msg "すでに処理済みです" :status status})
          :else
            (do
              (db/receive-check! {:id id :dest (rcv :receiver)})
              (ok {:msg "受け取りました"})))))

    ;; 口座入力API
    (POST "/check/checkin" []
      :tags ["receive"]
      :return {:msg s/Str :account_no s/Str :amount Long}
      :body [rcv {:receiver s/Str :account_no s/Str}]
      :summary "送金先口座登録"
      (let [count (db/confirm-check! {:dest (rcv :receiver) :acc_id (rcv :account_no)})
            check (first (db/get-check-by-dest {:dest (rcv :receiver) :status 2}))]
        (if (not= 0 count)
          (ok {:msg "設定しました" :account_no (rcv :account_no) :amount (check :amount)})
          (bad-request {:msg "処理できない状態か、登録のないメールアドレスです" :request rcv}))))

    ;; 小切手決済
    (POST "/check/settle" []
      :tags ["receive"]
      :return {:msg s/Str}
      :body [rcv {:receiver s/Str}]
      :summary "小切手の入金予約"
      (let [count (db/ready-check! {:dest (rcv :receiver)})]
        (if (not= 0 count)
          (do
            (send-sms "承認が必要です")
            (ok {:msg "入金予約を行いました。承認後、振込が行われます"}))
          (bad-request {:msg "処理出来ない状態か、登録のないメールアドレスです"}))))

    ;; 小切手承認
    (POST "/check/approve" []
      :tags ["receive"]
      :return {:msg s/Str :count s/Int}
      :form-params [auth_token :- s/Str]
      :summary "小切手の承認"
      (wrap-api
        (fn []
          (log/info (str "/check/approve:" auth_token))
          ;; 振込予約
          (let [count (for [check (db/get-checks-by-status {:status 3})]
                        (transfer auth_token check))]
            ;; 振込承認
            (approve auth_token)
            (ok {:msg "振込が行われました" :count count})))))

    ;; 小切手一覧
    (GET "/checks" []
      :tags ["list"]
      :return [(s/maybe Check)]
      :query-params [{status :- s/Int nil}]
      :summary "小切手一覧"
      (if status
        (db/get-checks-by-status {:status status})
        (db/get-checks)))))
