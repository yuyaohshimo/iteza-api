require File.expand_path(File.dirname(__FILE__) + '/../modules/send_mail')

class DepositRequestJob
  include SendMail

  def receive(message, params)
    receiver = message.from_addrs[0]
    send_deposit_request(receiver)
  end
end