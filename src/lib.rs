mod lten;
mod tensor;

pub use tensor::DType;
pub use tensor::Device;

#[derive(thiserror::Error, Debug)]
pub enum Error {
    #[error("tensor operator error")]
    LynnError(String),
}

type Result<T> = std::result::Result<T, Error>;

pub use tensor::Tensor;
