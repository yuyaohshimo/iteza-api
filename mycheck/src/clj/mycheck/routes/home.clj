(ns mycheck.routes.home
  (:require [mycheck.layout :as layout]
            [compojure.core :refer [defroutes GET]]
            [ring.util.http-response :as response]
            [clojure.java.io :as io]
            [config.core :refer [env]]))

(defn callback-url [path]
  [:safe
    (str (:api-endpoint env)
      "/oauth/authorize?"
      "redirect_uri="
      (:url env) ":" (:port env) path "&"
      "client_id=" (:app-token env)
      "&" "response_type=token")])

(defn home-page []
  (layout/render
    "home.html" {:authurl (callback-url "/auth")
                 :approveurl (callback-url "/approve")}))

(defn auth-page []
  (layout/render "auth.html"))

(defn about-page []
  (layout/render "about.html"))

(defn hello-page []
  (layout/render-xml "sample.xml"))

(defn approve-page []
  (layout/render "approve.html"))

(defn regist-page []
  (layout/render "regist.html"))

(defn docs-page []
  (layout/render "docs.html"
           {:docs (-> "docs/docs.md" io/resource slurp)}))

(defroutes home-routes
  (GET "/" [] (home-page))
  (GET "/auth" [] (auth-page))
  (GET "/about" [] (about-page))
  (GET "/docs" [] (docs-page))
  (GET "/hello" [] (hello-page))
  (GET "/approve" [] (approve-page))
  (GET "/regist" [] (regist-page))
  (GET "/coupon" []
    (layout/render "coupon.html")))
