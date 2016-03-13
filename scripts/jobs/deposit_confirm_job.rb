require 'rest-client'
require 'json'

require File.expand_path(File.dirname(__FILE__) + '/../modules/send_mail')

class DepositConfirmJob
  include SendMail

  API_URL = "http://www.fiftyriver.net:3000/api/check/settle"

  def receive(message, params)
    receiver = message.from_addrs[0]

    begin
      response = RestClient.post API_URL,
        { receiver: receiver }.to_json,
        content_type: :json,
        accept: :json

      puts JSON.parse(response)

      send_deposit_success(receiver)
    rescue => e
      puts e.message
    end
  end
end