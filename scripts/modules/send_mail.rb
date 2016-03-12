require 'active_support/concern'

module SendMail
  extend ActiveSupport::Concern

  included do
    def send_receive_success(from)
      deliver(from, 'templates/receive_success.txt')
    end

    def deliver(from, template_path)
      mail = Mail.new do
        from    'get@checky.me'
        to      from
        subject 'Mail from Checky'
        body    File.read(template_path)
      end
      mail.charset = 'utf-8'
      mail.deliver
    end
  end
end