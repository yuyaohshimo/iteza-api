(ns mycheck.routes.home
  (:require [mycheck.layout :as layout]
            [compojure.core :refer [defroutes GET]]
            [ring.util.http-response :as response]
            [clojure.java.io :as io]
            [config.core :refer [env]]))

(def authurl
  [:safe
    (str (:api-endpoint env)
      "/oauth/authorize?"
      "redirect_uri="
      (:url env) ":" (:port env) "/auth" "&"
      "client_id=" (:app-token env)
      "&" "response_type=token")])

(defn home-page []
  (layout/render
    "home.html" {:authurl authurl}))

(defn auth-page []
  (layout/render "auth.html"))

(defn about-page []
  (layout/render "about.html"))

(defn docs-page []
  (layout/render "docs.html"
           {:docs (-> "docs/docs.md" io/resource slurp)}))

(defroutes home-routes
  (GET "/" [] (home-page))
  (GET "/auth" [] (auth-page))
  (GET "/about" [] (about-page))
  (GET "/docs" [] (docs-page)))
