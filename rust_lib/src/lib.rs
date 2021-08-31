use reqwest::blocking;

#[cxx::bridge]
mod ffi {
    #[namespace = "rust_ffi"]
    extern "Rust" {
       fn get_msg_hash(message: &String) -> String;
       fn post_request(request_url: &String, body: &String) -> String; // Vec<[String;2]>)
    }
}

pub fn get_msg_hash(message: &String) -> String {
    hex::encode(
        openssl::sha::sha512(message.as_bytes())
    ).to_string()
}

pub fn post_request(request_url: &String, body: &String) -> String {
    let client = blocking::Client::new();

    let res = client.post(request_url)
                    .body(body.clone())
                    .send();

    match res {
        Ok(response) => {
            response.text().unwrap()
        },
        Err(err) => {
            panic!("{:?}", err);
        }
    }
}

