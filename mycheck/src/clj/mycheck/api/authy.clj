(ns mycheck.api.authy
  (:require [org.httpkit.client :as http]
            [config.core :refer [env]])
  (:use [mycheck.api.core]
        [clojure.string :only [join blank?]]))

(def base "http://api.authy.com/protected/json")

(def authy-key (:authy-key env))

(defn api-url [api & actions]
  (let [act (join "&" actions)]
    (str base api "?api_key=" authy-key (if-not (blank? act) "&") act)))

(defn wrap [defer]
  (get-json defer))

(defn new-user [user]
  (wrap
    (http/post (api-url "/users/new") {:query-params {:user user}})))

(defn send-sms [id force]
  (wrap
    (http/get (api-url (str "/sms/" id) (if force "force=true")))))

(defn verify-token [id token]
  (wrap
    (http/get (api-url (join "/" ["/verify" token id])))))
